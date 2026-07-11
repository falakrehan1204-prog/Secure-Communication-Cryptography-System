// server.js — SecureCrypt API Gateway
//
// Architecture:
//   Browser  --HTTP-->  server.js (Node/Express)  --TCP socket-->  server.cpp
//
// The actual AES encryption/decryption is done here in Node using the
// built-in `crypto` module (audited, standard AES-256/192/128-CBC).
// The C++ engine (server.cpp) is used to compute a SHA-256 integrity hash
// of the ciphertext over a raw TCP socket, demonstrating the socket
// programming + SHA-256 pieces of the project. If the C++ engine isn't
// running, encryption/decryption still work fine — the hash is just omitted.

const express = require('express');
const crypto = require('crypto');
const net = require('net');
const cors = require('cors');
const path = require('path');

const app = express();

// Middleware
app.use(cors());
app.use(express.json());

// Serve the static front-end HTML/CSS/JS files directly from the root folder
app.use(express.static(__dirname));

const ALGO_MAP = {
    'AES-256': { cipher: 'aes-256-cbc', keyLen: 32 },
    'AES-192': { cipher: 'aes-192-cbc', keyLen: 24 },
    'AES-128': { cipher: 'aes-128-cbc', keyLen: 16 },
};

// Derive a fixed-length key from the user's passphrase using SHA-256
function deriveKey(passphrase, keyLen) {
    return crypto.createHash('sha256').update(String(passphrase)).digest().slice(0, keyLen);
}

// Ask the C++ engine to compute a SHA-256 integrity hash over TCP.
// Optional — if the C++ engine isn't running, we resolve(null) instead of failing the request.
function getIntegrityHash(data) {
    return new Promise((resolve) => {
        const client = new net.Socket();
        let settled = false;
        const finish = (value) => {
            if (settled) return;
            settled = true;
            resolve(value);
            client.destroy();
        };

        client.setTimeout(1500);
        client.connect(8080, '127.0.0.1', () => {
            client.write(JSON.stringify({ action: 'hash', data }) + '\n');
        });
        client.on('data', (chunk) => {
            try {
                const parsed = JSON.parse(chunk.toString());
                finish(parsed.status === 'success' ? parsed.data : null);
            } catch {
                finish(null);
            }
        });
        client.on('error', () => finish(null));
        client.on('timeout', () => finish(null));
    });
}

// Route for handling Encryption requests
app.post('/api/encrypt', async (req, res) => {
    try {
        const { message, algorithm, key } = req.body;
        if (!message || !key) {
            return res.status(400).json({ error: 'Message and secret key are required.' });
        }

        const config = ALGO_MAP[algorithm] || ALGO_MAP['AES-256'];
        const derivedKey = deriveKey(key, config.keyLen);
        const iv = crypto.randomBytes(16);

        const cipher = crypto.createCipheriv(config.cipher, derivedKey, iv);
        const encrypted = Buffer.concat([cipher.update(String(message), 'utf8'), cipher.final()]);

        // Pack IV + ciphertext together so decrypt can recover the IV
        const combined = `${iv.toString('base64')}:${encrypted.toString('base64')}`;

        const hash = await getIntegrityHash(combined);
        res.json({ result: combined, hash });
    } catch (err) {
        res.status(500).json({ error: 'Encryption failed: ' + err.message });
    }
});

// Route for handling Decryption requests
app.post('/api/decrypt', async (req, res) => {
    try {
        const { message, algorithm, key } = req.body;
        if (!message || !key) {
            return res.status(400).json({ error: 'Encrypted message and secret key are required.' });
        }

        const [ivB64, cipherB64] = String(message).split(':');
        if (!ivB64 || !cipherB64) {
            return res.status(400).json({ error: 'Invalid encrypted message format.' });
        }

        const config = ALGO_MAP[algorithm] || ALGO_MAP['AES-256'];
        const derivedKey = deriveKey(key, config.keyLen);
        const iv = Buffer.from(ivB64, 'base64');

        const decipher = crypto.createDecipheriv(config.cipher, derivedKey, iv);
        const decrypted = Buffer.concat([
            decipher.update(Buffer.from(cipherB64, 'base64')),
            decipher.final(),
        ]);

        res.json({ result: decrypted.toString('utf8') });
    } catch (err) {
        // Wrong key or corrupted/mismatched ciphertext throws inside decipher.final()
        res.status(400).json({ error: 'Decryption failed. Check your secret key and encrypted message.' });
    }
});

// Fallback route to serve the homepage for any unknown path
app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, 'index.html'));
});

// Start the Node web gateway on Port 3000
app.listen(3000, () => {
    console.log('🚀 SecureCrypt Gateway running at http://localhost:3000');
});
