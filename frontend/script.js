// grab all the dom elements we need
const loginForm = document.getElementById('loginForm');
const registerForm = document.getElementById('registerForm');
const weatherApp = document.getElementById('weatherApp');
const toggleBtn = document.getElementById('toggleBtn');

const loginMessage = document.getElementById('loginMessage');
const registerMessage = document.getElementById('registerMessage');
const weatherResult = document.getElementById('weatherResult');
const currentUserSpan = document.getElementById('currentUser');

const loginBtn = document.getElementById('loginBtn');
const registerBtn = document.getElementById('registerBtn');
const checkBtn = document.getElementById('checkBtn');
const logoutBtn = document.getElementById('logoutBtn');

let isLoginMode = true;
let currentUsername = '';

// switch between login and register screens
function toggleAuth() {
  isLoginMode = !isLoginMode;
  
  if (isLoginMode) {
    loginForm.style.display = 'block';
    registerForm.style.display = 'none';
    toggleBtn.textContent = "Don't have an account? Sign up";
    clearMessages();
  } else {
    loginForm.style.display = 'none';
    registerForm.style.display = 'block';
    toggleBtn.textContent = 'Already have an account? Login';
    clearMessages();
  }
}

// clear error/success messages
function clearMessages() {
  loginMessage.textContent = '';
  loginMessage.className = 'message';
  registerMessage.textContent = '';
  registerMessage.className = 'message';
}

// show a message to the user
function showMessage(element, text, type = 'error') {
  element.textContent = text;
  element.className = `message ${type}`;
}

// register a new account
registerBtn.addEventListener('click', async () => {
  const username = document.getElementById('regUsername').value.trim();
  const password = document.getElementById('regPassword').value.trim();
  const confirm = document.getElementById('regPasswordConfirm').value.trim();

  if (!username || !password || !confirm) {
    showMessage(registerMessage, 'Please fill in all fields', 'error');
    return;
  }

  if (username.length < 3) {
    showMessage(registerMessage, 'Username must be at least 3 characters', 'error');
    return;
  }

  if (username.length > 32) {
    showMessage(registerMessage, 'Username must be 32 characters or less', 'error');
    return;
  }

  if (password.length < 6) {
    showMessage(registerMessage, 'Password must be at least 6 characters', 'error');
    return;
  }

  if (password !== confirm) {
    showMessage(registerMessage, 'Passwords do not match', 'error');
    return;
  }

  registerBtn.disabled = true;
  registerBtn.textContent = 'Creating account...';

  try {
    const res = await fetch('/register', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ username, password })
    });

    const data = await res.json();

    if (res.ok && data.status === "success") {
      showMessage(registerMessage, 'Account created! Switching to login...', 'success');
      
      document.getElementById('regUsername').value = '';
      document.getElementById('regPassword').value = '';
      document.getElementById('regPasswordConfirm').value = '';

      setTimeout(() => {
        toggleAuth();
        document.getElementById('username').value = username;
        document.getElementById('password').focus();
      }, 1500);
    } else {
      showMessage(registerMessage, `${data.message || 'Registration failed'}`, 'error');
    }
  } catch (err) {
    showMessage(registerMessage, 'Server error - please try again', 'error');
    console.error('Register error:', err);
  } finally {
    registerBtn.disabled = false;
    registerBtn.textContent = 'Register';
  }
});

// log in to existing account
loginBtn.addEventListener('click', async () => {
  const username = document.getElementById('username').value.trim();
  const password = document.getElementById('password').value.trim();

  if (!username || !password) {
    showMessage(loginMessage, 'Please enter both username and password', 'error');
    return;
  }

  loginBtn.disabled = true;
  loginBtn.textContent = 'Logging in...';

  try {
    const res = await fetch('/login', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ username, password })
    });

    const data = await res.json();

    if (res.ok && data.status === "success") {
      loginForm.style.display = 'none';
      registerForm.style.display = 'none';
      weatherApp.style.display = 'block';
      currentUsername = username;
      currentUserSpan.textContent = username;
      
      document.getElementById('username').value = '';
      document.getElementById('password').value = '';
      
      console.log('User logged in:', username);
    } else {
      showMessage(loginMessage, `${data.message || 'Invalid credentials'}`, 'error');
    }
  } catch (err) {
    showMessage(loginMessage, 'Server error - please try again', 'error');
    console.error('Login error:', err);
  } finally {
    loginBtn.disabled = false;
    loginBtn.textContent = 'Login';
  }
});

// fetch weather for a city
checkBtn.addEventListener('click', async () => {
  const city = document.getElementById('city').value.trim();
  
  if (!city) {
    weatherResult.innerHTML = '<p class="error">Please enter a city name</p>';
    return;
  }

  if (city.length > 50) {
    weatherResult.innerHTML = '<p class="error">City name is too long</p>';
    return;
  }

  checkBtn.disabled = true;
  checkBtn.textContent = 'Loading...';
  weatherResult.innerHTML = '<p>Fetching weather data...</p>';

  try {
    const res = await fetch(`/weather/${encodeURIComponent(city)}`);
    const data = await res.json();

    if (data.main && data.weather) {
      const temp = Math.round(data.main.temp);
      const feelsLike = Math.round(data.main.feels_like);
      const humidity = data.main.humidity;
      const windSpeed = data.wind.speed;
      const description = data.weather[0].description;

      const html = `
        <div class="weather-card">
          <h3>${data.name}, ${data.sys.country}</h3>
          <p class="temperature">${temp}°F</p>
          <p class="description">${description.charAt(0).toUpperCase() + description.slice(1)}</p>
          <div class="weather-details">
            <div class="detail">
              <span class="label">Feels like:</span>
              <span class="value">${feelsLike}°F</span>
            </div>
            <div class="detail">
              <span class="label">Humidity:</span>
              <span class="value">${humidity}%</span>
            </div>
            <div class="detail">
              <span class="label">Wind:</span>
              <span class="value">${windSpeed} mph</span>
            </div>
          </div>
        </div>
      `;
      weatherResult.innerHTML = html;
    } else if (data.error) {
      weatherResult.innerHTML = `<p class="error">${data.error}</p>`;
    } else {
      weatherResult.innerHTML = '<p class="error">Unable to parse weather data</p>';
    }
  } catch (err) {
    weatherResult.innerHTML = '<p class="error">Failed to fetch weather data</p>';
    console.error('Weather error:', err);
  } finally {
    checkBtn.disabled = false;
    checkBtn.textContent = 'Get Weather';
  }
});

// log out and go back to login screen
logoutBtn.addEventListener('click', () => {
  loginForm.style.display = 'block';
  registerForm.style.display = 'none';
  weatherApp.style.display = 'none';
  
  document.getElementById('username').value = '';
  document.getElementById('password').value = '';
  document.getElementById('city').value = '';
  weatherResult.innerHTML = '';
  currentUsername = '';
  
  isLoginMode = true;
  toggleBtn.textContent = "Don't have an account? Sign up";
  clearMessages();
  
  console.log('User logged out');
});

toggleBtn.addEventListener('click', toggleAuth);

// allow pressing enter to submit forms
document.getElementById('username').addEventListener('keypress', (e) => {
  if (e.key === 'Enter' && isLoginMode) {
    document.getElementById('password').focus();
  }
});

document.getElementById('password').addEventListener('keypress', (e) => {
  if (e.key === 'Enter' && isLoginMode) {
    loginBtn.click();
  }
});

document.getElementById('regUsername').addEventListener('keypress', (e) => {
  if (e.key === 'Enter' && !isLoginMode) {
    document.getElementById('regPassword').focus();
  }
});

document.getElementById('regPassword').addEventListener('keypress', (e) => {
  if (e.key === 'Enter' && !isLoginMode) {
    document.getElementById('regPasswordConfirm').focus();
  }
});

document.getElementById('regPasswordConfirm').addEventListener('keypress', (e) => {
  if (e.key === 'Enter' && !isLoginMode) {
    registerBtn.click();
  }
});

document.getElementById('city').addEventListener('keypress', (e) => {
  if (e.key === 'Enter') {
    checkBtn.click();
  }
});

console.log('Weather app ready');