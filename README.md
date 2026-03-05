# Weather App

A full-stack weather application built with C++ backend and JavaScript frontend. Register an account, log in, and check weather for any city using the free OpenWeatherMap API.

## What You Get

- User registration and login with password hashing
- Real-time weather data for any city
- Clean, responsive UI
- SQLite database for user storage

## Setup

### Prerequisites

- MSYS2 with UCRT64 environment
- GCC compiler
- Git

### Installation

1. Clone the repo:
```bash
git clone https://github.com/Meuxo/weather-fullstack.git
cd weather-fullstack/backend
```

2. Install dependencies (if not already installed):
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc git
```

3. Make sure you have these files in the `backend/` folder:
   - `main.cpp`
   - `sqlite3.c` and `sqlite3.h`
   - `httplib.h`
   - `json.hpp`
   - `frontend/` folder with HTML/CSS/JS files

### Compile

```bash
gcc -c sqlite3.c
g++ -std=c++17 -O2 main.cpp sqlite3.o -lws2_32 -lwsock32 -pthread -o server.exe
```

### Run

```bash
./server.exe
```

Open your browser to `http://localhost:8081/` and you're ready to go.

## How to Use

1. Register - Create a new account with a username and password
2. Login - Sign in with your credentials
3. Check Weather - Enter any city name and get current weather data including temperature, humidity, and wind speed

## Features

- User authentication with hashed passwords
- Weather API integration
- Form validation
- Responsive design
- Enter key support for forms

## Tech Stack

- Backend: C++17, httplib, SQLite
- Frontend: HTML, CSS, JavaScript
- Database: SQLite
- API: OpenWeatherMap

## Project Structure

```
backend/
├── main.cpp           # Server and API endpoints
├── sqlite3.c/h        # Database
├── httplib.h          # HTTP server library
├── json.hpp           # JSON parsing
├── database.db        # User database (created on first run)
├── server.exe         # Compiled executable
└── frontend/
    ├── index.html     # Login/register UI
    ├── script.js      # JavaScript logic
    └── style.css      # Styling
```

# Notes

- Passwords are hashed using `std::hash` (basic - use bcrypt for production)
- The server runs on port 8081
- Frontend files are served from the `frontend/` folder
- Database is created automatically on startup

## Future Improvements

- Better password hashing (bcrypt/argon2)
- JWT token authentication
- Multiple user profiles
- Weather history
- Favorite cities list
