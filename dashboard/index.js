
require('dotenv').config();
console.log('Node.js application started');

// Require the necessary packages
const express = require('express');
const bodyParser = require('body-parser');
const cookieParser = require('cookie-parser');
const path = require('path');
const http = require('http');
const AWS = require('aws-sdk');
const WebSocket = require('ws');
const awsIot = require('aws-iot-device-sdk');

// AWS Configuration
const AWS_REGION = 'ap-southeast-2';
const COGNITO_IDENTITY_POOL_ID = 'ap-southeast-2:9270ed60-7a98-4814-b416-2e0b4879bbfa';
const COGNITO_USER_POOL_ID = 'ap-southeast-2_BocMacrEF';
const COGNITO_CLIENT_ID = '7s5b865f9n29egaf83ps0n51og';
const IOT_ENDPOINT = 'a1zx1lood4k4kc-ats.iot.ap-southeast-2.amazonaws.com';

// Configure AWS SDK
AWS.config.region = AWS_REGION;

// Create Cognito clients
const cognitoIdentity = new AWS.CognitoIdentity();
const cognitoISP = new AWS.CognitoIdentityServiceProvider();

// Initialize Express app
const app = express();
app.use(express.json());
app.use(bodyParser.json());
app.use(cookieParser());
app.use(express.static(path.join(__dirname, 'public')));

// Create HTTP and WebSocket server
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// --- Routes ---

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.post('/login', (req, res) => {
    const { username, password } = req.body;
    if (!username || !password) {
        return res.status(400).json({ success: false, message: 'Username and password are required.' });
    }

    cognitoISP.initiateAuth({
        AuthFlow: 'USER_PASSWORD_AUTH',
        ClientId: COGNITO_CLIENT_ID,
        AuthParameters: { USERNAME: username, PASSWORD: password },
    }, (err, authData) => {
        if (err) {
            console.error("Error during login:", err);
            return res.status(401).json({ success: false, message: 'Invalid username or password.' });
        }
        res.cookie('IdToken', authData.AuthenticationResult.IdToken, { httpOnly: true, secure: process.env.NODE_ENV === 'production', sameSite: 'strict' });
        res.cookie('AccessToken', authData.AuthenticationResult.AccessToken, { httpOnly: true, secure: process.env.NODE_ENV === 'production', sameSite: 'strict' });
        res.json({ success: true, message: 'Login successful!' });
    });
});

const requireAuth = (req, res, next) => {
    if (!req.cookies.IdToken) return res.redirect('/');
    next();
};

app.get('/dashboard', requireAuth, (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'dashboard.html'));
});

app.get('/api/credentials', requireAuth, (req, res) => {
    const idToken = req.cookies.IdToken;
    const logins = { [`cognito-idp.${AWS_REGION}.amazonaws.com/${COGNITO_USER_POOL_ID}`]: idToken };

    cognitoIdentity.getId({ IdentityPoolId: COGNITO_IDENTITY_POOL_ID, Logins: logins }, (err, identityData) => {
        if (err) {
            console.error("Error getting Cognito Identity ID:", err);
            return res.status(500).json({ success: false, message: 'Failed to get Identity ID.' });
        }

        cognitoIdentity.getCredentialsForIdentity({ IdentityId: identityData.IdentityId, Logins: logins }, (err, credentialsData) => {
            if (err) {
                console.error("Error getting credentials for identity:", err);
                return res.status(500).json({ success: false, message: 'Failed to get AWS credentials.' });
            }
            res.json({
                success: true,
                credentials: {
                    accessKeyId: credentialsData.Credentials.AccessKeyId,
                    // *** BUG FIX: The SDK expects the property name to be 'secretKey' not 'secretAccessKey' ***
                    secretKey: credentialsData.Credentials.SecretKey,
                    sessionToken: credentialsData.Credentials.SessionToken,
                    identityId: identityData.IdentityId
                }
            });
        });
    });
});

app.post('/logout', (req, res) => {
    const accessToken = req.cookies.AccessToken;
    if (accessToken) {
        cognitoISP.globalSignOut({ AccessToken: accessToken }, (err) => {
            if (err) console.error("Error during global sign out:", err);
            else console.log("Global sign out successful.");
        });
    }
    res.clearCookie('IdToken');
    res.clearCookie('AccessToken');
    res.status(200).json({ success: true, message: 'Logged out successfully' });
});

// --- WebSocket and MQTT Logic ---

wss.on('connection', (ws) => {
    console.log('Frontend WebSocket client connected');
    let clientMqtt = null;

    ws.on('message', (message) => {
        try {
            const data = JSON.parse(message);
            if (data.type === 'start-mqtt' && data.credentials) {
                if (clientMqtt) {
                    console.log('Cleaning up previous MQTT client for this WebSocket.');
                    clientMqtt.removeAllListeners();
                    clientMqtt.end(true);
                }
                clientMqtt = connectMqttForClient(data.credentials, ws);
            }
        } catch (e) {
            console.error("Invalid message from client:", e);
        }
    });

    ws.on('close', () => {
        console.log('Frontend WebSocket client disconnected');
        if (clientMqtt) {
            console.log('Closing MQTT client for disconnected WebSocket.');
            clientMqtt.removeAllListeners();
            clientMqtt.end(true);
            clientMqtt = null;
        }
    });

    ws.on('error', (error) => {
        console.error('Frontend WebSocket error:', error);
    });
});

function connectMqttForClient(credentials, ws) {
    console.log("Creating new MQTT client for WebSocket with Identity ID:", credentials.identityId);
    const newClient = awsIot.device({
        protocol: 'wss',
        host: IOT_ENDPOINT,
        accessKeyId: credentials.accessKeyId,
        secretKey: credentials.secretKey, // This line now works correctly
        sessionToken: credentials.sessionToken,
        region: AWS_REGION,
        // *** FIX: Set the clientId to the user's unique identity ***
        clientId: credentials.identityId
    });

    newClient.on('connect', () => {
        console.log('MQTT Client Connected with ClientId:', credentials.identityId);
        const topic = 'webapp/data';
        console.log(`Subscribing to topic: ${topic}`);
        newClient.subscribe(topic, (err) => {
            if (err) console.error(`MQTT Subscription error for topic ${topic}:`, err);
        });
    });

    newClient.on('message', (topic, payload) => {
        const messageStr = payload.toString();
        if (ws.readyState === WebSocket.OPEN) {
            ws.send(messageStr);
        }
    });

    newClient.on('error', (err) => {
        console.error('MQTT Client Error:', err);
    });

    newClient.on('close', () => {
        console.log('MQTT Client instance has closed.');
    });

    return newClient;
}

process.on('SIGINT', () => {
    console.log('Shutting down gracefully...');
    wss.clients.forEach(ws => {
        ws.terminate();
    });
    server.close(() => {
        console.log('HTTP server closed.');
        process.exit(0);
    });
});

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
    console.log(`Node.js server listening on port ${PORT}`);
});
