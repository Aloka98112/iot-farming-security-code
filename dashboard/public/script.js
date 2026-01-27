console.log('Frontend script.js loaded');

document.addEventListener('DOMContentLoaded', () => {
    // --- Check which page we are on ---
    const isLoginPage = window.location.pathname === '/' || window.location.pathname === '/index.html';
    const isDashboardPage = window.location.pathname.startsWith('/dashboard');

    if (isLoginPage) {
        setupLoginPage();
    } else if (isDashboardPage) {
        setupDashboardPage();
    }
});

// --- LOGIN PAGE SCRIPT --- //
function setupLoginPage() {
    const loginButton = document.getElementById('login-button');
    if (loginButton) {
        loginButton.addEventListener('click', handleLogin);
    }
}

async function handleLogin() {
    const usernameInput = document.getElementById('username');
    const passwordInput = document.getElementById('password');
    const loginMessage = document.getElementById('login-message');
    const loginButton = document.getElementById('login-button');

    const username = usernameInput.value;
    const password = passwordInput.value;

    if (!username || !password) {
        loginMessage.textContent = 'Please enter username and password.';
        return;
    }

    loginMessage.textContent = '';
    loginButton.disabled = true;
    loginButton.textContent = 'Logging in...';

    try {
        const response = await fetch('/login', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, password }),
        });

        const result = await response.json();

        if (result.success) {
            console.log('Login successful');
            // Redirect to the dashboard. The browser will handle the secure cookie.
            window.location.href = '/dashboard';
        } else {
            loginMessage.textContent = result.message || 'Login failed.';
        }
    } catch (error) {
        console.error('Error during login fetch:', error);
        loginMessage.textContent = 'An error occurred. Please try again.';
    } finally {
        loginButton.disabled = false;
        loginButton.textContent = 'Log In';
    }
}

// --- DASHBOARD PAGE SCRIPT --- //

function setupDashboardPage() {
    const logoutButton = document.getElementById('logout-button');
    if (logoutButton) {
        logoutButton.addEventListener('click', handleLogout);
    }

    const connectionStatusElement = document.getElementById('connection-status');
    let websocket;

    // 1. Fetch credentials from the secure endpoint
    fetch('/api/credentials')
        .then(response => {
            if (!response.ok) {
                // If we get an error (e.g., 401 Unauthorized), the cookie is bad. Redirect to login.
                throw new Error('Not authenticated');
            }
            return response.json();
        })
        .then(result => {
            if (result.success) {
                // 2. We have credentials, now connect WebSocket to our server
                connectWebSocket(result.credentials);
            } else {
                throw new Error(result.message || 'Failed to get credentials.');
            }
        })
        .catch(error => {
            console.error('Auth Error:', error.message);
            // On any auth error, redirect to the login page
            window.location.href = '/';
        });

    function connectWebSocket(credentials) {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        websocket = new WebSocket(`${protocol}//${window.location.host}`);

        websocket.addEventListener('open', () => {
            console.log('WebSocket connected to server.');
            connectionStatusElement.textContent = 'Connected';
            connectionStatusElement.className = 'inline-block px-4 py-2 text-sm font-semibold rounded-full bg-green-200 text-green-800';
            
            // 3. Send credentials to the server to start the MQTT client
            websocket.send(JSON.stringify({ type: 'start-mqtt', credentials }));
        });

        websocket.addEventListener('message', (event) => {
            try {
                const data = JSON.parse(event.data);
                updateDashboardUI(data);
            } catch (e) {
                console.error('Error parsing message from server:', e);
            }
        });

        websocket.addEventListener('error', (event) => {
            console.error('WebSocket error:', event);
            connectionStatusElement.textContent = 'Error';
            connectionStatusElement.className = 'inline-block px-4 py-2 text-sm font-semibold rounded-full bg-red-200 text-red-800';
        });

        websocket.addEventListener('close', () => {
            console.log('WebSocket disconnected.');
            connectionStatusElement.textContent = 'Disconnected';
            connectionStatusElement.className = 'inline-block px-4 py-2 text-sm font-semibold rounded-full bg-gray-300 text-gray-700';
        });
    }
}

function updateDashboardUI(data) {
    const moistureElement = document.getElementById('moisture-value');
    const temperatureElement = document.getElementById('temperature-value');
    const humidityElement = document.getElementById('humidity-value');
    const pumpStatusElement = document.getElementById('pump-status');

    if (moistureElement && data.moisture_percent !== undefined) {
        moistureElement.textContent = `${data.moisture_percent} %`;
    }
    if (temperatureElement && data.temperature_c !== undefined) {
        temperatureElement.textContent = `${data.temperature_c} °C`;
    }
    if (humidityElement && data.humidity_percent !== undefined) {
        humidityElement.textContent = `${data.humidity_percent} %`;
    }
    if (pumpStatusElement && data.watering_active !== undefined) {
        pumpStatusElement.textContent = data.watering_active ? 'ONLINE' : 'OFFLINE';
        pumpStatusElement.className = data.watering_active
            ? 'inline-block px-4 py-2 text-sm font-semibold rounded-full bg-green-200 text-green-800'
            : 'inline-block px-4 py-2 text-sm font-semibold rounded-full bg-gray-300 text-gray-700';
    }
}

async function handleLogout() {
    try {
        await fetch('/logout', { method: 'POST' });
        console.log('Logout successful');
    } catch (error) {
        console.error('Logout failed:', error);
    } finally {
        // Always redirect to login page after attempting logout
        window.location.href = '/';
    }
}
