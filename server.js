// server.js
const express = require('express');
const net = require('net');
const cors = require('cors');
const path = require('path');

const app = express();

// Middleware
app.use(cors());
app.use(express.json());

// Serve your static front-end HTML/CSS/JS files directly from the root folder
app.use(express.static(__dirname));

// Helper function to send data over raw TCP sockets to your C++ server
function communicateWithCpp(payload, callback) {
    const client = new net.Socket();
    
    // Connects to local C++ server listening on port 8080
    client.connect(8080, '127.0.0.1', () => {
        client.write(JSON.stringify(payload) + "\n"); // Append newline as a delimiter
    });

    client.on('data', (data) => {
        callback(null, data.toString());
        client.destroy(); // Close the socket connection
    });

    client.on('error', (err) => {
        callback(err, null);
    });
}

// Route for handling Encryption requests
app.post('/api/encrypt', (req, res) => {
    const payload = { action: 'encrypt', ...req.body };
    communicateWithCpp(payload, (err, responseFromCpp) => {
        if (err) return res.status(500).json({ error: "Cryptography Engine Offline" });
        try {
            const parsed = JSON.parse(responseFromCpp);
            res.json({ result: parsed.data });
        } catch {
            res.json({ result: responseFromCpp.trim() });
        }
    });
});

// Route for handling Decryption requests
app.post('/api/decrypt', (req, res) => {
    const payload = { action: 'decrypt', ...req.body };
    communicateWithCpp(payload, (err, responseFromCpp) => {
        if (err) return res.status(500).json({ error: "Cryptography Engine Offline" });
        try {
            const parsed = JSON.parse(responseFromCpp);
            res.json({ result: parsed.data });
        } catch {
            res.json({ result: responseFromCpp.trim() });
        }
    });
});

// Fallback route to serve your index.html homepage
app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, 'index.html'));
});

// Start the Node web gateway on Port 3000
app.listen(3000, () => {
    console.log('🚀 SecureCrypt Gateway running at http://localhost:3000');
});