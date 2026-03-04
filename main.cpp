#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <functional>
#include <algorithm>
#include "httplib.h"
#include "json.hpp"
#include "sqlite3.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

const std::string apiKey = "bcd3dd75e9f954b1b4907392bb862af8";

// hash passwords - not super secure but for testing purposes
std::string hashPassword(const std::string& password) {
    std::hash<std::string> hasher;
    size_t hash_value = hasher(password);
    return std::to_string(hash_value);
}

// set up the database when the server starts
void initDB(sqlite3*& db) {
    if (sqlite3_open("database.db", &db)) {
        std::cerr << "ERROR: Cannot open database\n";
        exit(1);
    }

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL ERROR: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        exit(1);
    }

    std::cout << "[✓] Database initialized successfully\n";
}

// add a new user to the database
bool registerUser(sqlite3* db, const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) {
        return false;
    }

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    std::string hashedPassword = hashPassword(password);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hashedPassword.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    
    if (!success) {
        std::cerr << "Register error: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return success;
}

// check if the username and password are correct
bool authenticateUser(sqlite3* db, const std::string& username, const std::string& password) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT password FROM users WHERE username = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    bool authenticated = false;
    std::string hashedPassword = hashPassword(password);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* dbPassword = sqlite3_column_text(stmt, 0);
        if (dbPassword != nullptr) {
            authenticated = (hashedPassword == reinterpret_cast<const char*>(dbPassword));
        }
    }

    sqlite3_finalize(stmt);
    return authenticated;
}

// read a file from disk
std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "WARNING: Cannot read file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// find the frontend folder - checks a few common locations
fs::path getFrontendPath() {
    fs::path candidates[] = {
        fs::current_path() / "frontend",
        fs::current_path() / ".." / "frontend",
        fs::current_path() / "." / "frontend",
    };

    for (const auto& candidate : candidates) {
        fs::path indexPath = candidate / "index.html";
        if (fs::exists(indexPath)) {
            return fs::absolute(candidate);
        }
    }

    return fs::current_path() / "frontend";
}

// check if a string ends with a suffix
bool endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

// get the right content type for different file types
std::string getMimeType(const std::string& filePath) {
    if (endsWith(filePath, ".html")) return "text/html; charset=utf-8";
    if (endsWith(filePath, ".css")) return "text/css; charset=utf-8";
    if (endsWith(filePath, ".js")) return "application/javascript; charset=utf-8";
    if (endsWith(filePath, ".json")) return "application/json";
    if (endsWith(filePath, ".png")) return "image/png";
    if (endsWith(filePath, ".jpg") || endsWith(filePath, ".jpeg")) return "image/jpeg";
    if (endsWith(filePath, ".svg")) return "image/svg+xml";
    return "text/plain; charset=utf-8";
}

// main server
int main() {
    std::cout << "================================\n";
    std::cout << "  Weather App Server (MSYS2)\n";
    std::cout << "================================\n\n";

    sqlite3* db;
    initDB(db);

    // find where the frontend files are
    fs::path frontendPath = getFrontendPath();
    std::cout << "[INFO] Frontend path: " << frontendPath.string() << "\n";

    if (!fs::exists(frontendPath / "index.html")) {
        std::cout << "[WARNING] index.html not found at: " << frontendPath << "\n";
        std::cout << "[TIP] Make sure frontend/ folder exists in your working directory\n";
    }

    std::cout << "[INFO] Current working directory: " << fs::current_path().string() << "\n\n";

    httplib::Server svr;

    // enable CORS so frontend can talk to backend
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization"},
        {"Cache-Control", "no-cache, no-store, must-revalidate"}
    });

    // handle CORS preflight requests
    svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 200;
    });

    // serve the main html file at /
    svr.Get("/", [frontendPath](const httplib::Request&, httplib::Response& res) {
        std::string indexPath = (frontendPath / "index.html").string();
        std::string content = readFile(indexPath);
        
        if (content.empty()) {
            res.status = 404;
            res.set_content(
                "<html><body><h1>404 Not Found</h1><p>index.html not found</p></body></html>",
                "text/html; charset=utf-8"
            );
            return;
        }
        res.set_content(content, "text/html; charset=utf-8");
    });

    // serve css, js, and other static files
    svr.set_mount_point("/", frontendPath.string());

    // registration endpoint
    svr.Post("/register", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);

            if (!j.contains("username") || !j.contains("password")) {
                res.status = 400;
                res.set_content(
                    R"({"status":"error","message":"Missing username or password"})",
                    "application/json"
                );
                return;
            }

            std::string username = j["username"];
            std::string password = j["password"];

            if (username.empty() || password.empty()) {
                res.status = 400;
                res.set_content(
                    R"({"status":"error","message":"Username and password cannot be empty"})",
                    "application/json"
                );
                return;
            }

            if (username.length() < 3 || username.length() > 32) {
                res.status = 400;
                res.set_content(
                    R"({"status":"error","message":"Username must be 3-32 characters"})",
                    "application/json"
                );
                return;
            }

            if (password.length() < 6) {
                res.status = 400;
                res.set_content(
                    R"({"status":"error","message":"Password must be at least 6 characters"})",
                    "application/json"
                );
                return;
            }

            if (registerUser(db, username, password)) {
                std::cout << "[✓] User registered: " << username << "\n";
                res.set_content(R"({"status":"success","message":"Account created"})", "application/json");
            } else {
                res.status = 400;
                res.set_content(
                    R"({"status":"error","message":"Username already taken or registration failed"})",
                    "application/json"
                );
            }
        } catch (const json::exception& e) {
            res.status = 400;
            res.set_content(
                R"({"status":"error","message":"Invalid JSON format"})",
                "application/json"
            );
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                R"({"status":"error","message":"Server error"})",
                "application/json"
            );
            std::cerr << "[ERROR] Register exception: " << e.what() << "\n";
        }
    });

    // login endpoint
    svr.Post("/login", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);

            if (!j.contains("username") || !j.contains("password")) {
                res.status = 400;
                res.set_content(
                    R"({"status":"error","message":"Missing username or password"})",
                    "application/json"
                );
                return;
            }

            std::string username = j["username"];
            std::string password = j["password"];

            if (authenticateUser(db, username, password)) {
                std::cout << "[✓] User logged in: " << username << "\n";
                res.set_content(R"({"status":"success","message":"Login successful"})", "application/json");
            } else {
                res.status = 401;
                res.set_content(
                    R"({"status":"error","message":"Invalid username or password"})",
                    "application/json"
                );
            }
        } catch (const json::exception& e) {
            res.status = 400;
            res.set_content(
                R"({"status":"error","message":"Invalid JSON format"})",
                "application/json"
            );
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                R"({"status":"error","message":"Server error"})",
                "application/json"
            );
            std::cerr << "[ERROR] Login exception: " << e.what() << "\n";
        }
    });

    // weather endpoint - calls openweathermap api
    svr.Get(R"(/weather/(\w+))", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string city = req.matches[1];

            if (city.empty() || city.length() > 50) {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid city name"})",
                    "application/json"
                );
                return;
            }

            httplib::Client cli("api.openweathermap.org", 80);
            cli.set_connection_timeout(5, 0);
            cli.set_read_timeout(5, 0);

            std::string path = "/data/2.5/weather?q=" + city +
                               "&appid=" + apiKey +
                               "&units=imperial";

            auto r = cli.Get(path.c_str());

            if (r && r->status == 200) {
                std::cout << "[✓] Weather request: " << city << "\n";
                res.set_content(r->body, "application/json");
            } else if (r && r->status == 404) {
                res.status = 404;
                res.set_content(
                    R"({"error":"City not found"})",
                    "application/json"
                );
            } else {
                res.status = 500;
                res.set_content(
                    R"({"error":"Failed to fetch weather data"})",
                    "application/json"
                );
            }
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                R"({"error":"Server error"})",
                "application/json"
            );
            std::cerr << "[ERROR] Weather exception: " << e.what() << "\n";
        }
    });

    // simple health check endpoint
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    // 404 handler for undefined endpoints
    svr.set_post_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        if (res.status == -1) {
            res.status = 404;
            res.set_content(
                R"({"error":"Endpoint not found"})",
                "application/json"
            );
        }
    });

    std::cout << "[INFO] Starting server...\n";
    std::cout << "[INFO] Listening on http://localhost:8081/\n";
    std::cout << "[INFO] Press Ctrl+C to stop\n\n";

    if (svr.listen("0.0.0.0", 8081)) {
        std::cout << "[✓] Server started successfully\n";
    } else {
        std::cerr << "[ERROR] Failed to start server\n";
        sqlite3_close(db);
        return 1;
    }

    sqlite3_close(db);
    return 0;
}