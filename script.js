// ==========================================================================
// SecureCrypt - Frontend Logic
// Talks to the Node.js gateway (server.js) at API_BASE.
// ==========================================================================
const API_BASE = 'http://localhost:3000';

// --------------------------------------------------------------------------
// Highlight the active nav link based on the current page
// --------------------------------------------------------------------------
(function highlightActiveNav() {
    const currentPage = window.location.pathname.split('/').pop() || 'index.html';
    document.querySelectorAll('header nav a').forEach((link) => {
        const linkPage = link.getAttribute('href');
        if (linkPage === currentPage) {
            link.classList.add('active');
        }
    });
})();

// --------------------------------------------------------------------------
// Small helpers
// --------------------------------------------------------------------------
function showMessage(el, text, type) {
    if (!el) return;
    el.textContent = text;
    el.className = 'form-message' + (type ? ' ' + type : '');
}

function setLoading(button, isLoading, defaultText) {
    if (!button) return;
    button.disabled = isLoading;
    button.textContent = isLoading ? 'Please wait...' : defaultText;
}

// --------------------------------------------------------------------------
// ENCRYPT PAGE
// --------------------------------------------------------------------------
const encryptForm = document.getElementById('encryptForm');
if (encryptForm) {
    const messageInput = document.getElementById('messageInput');
    const algorithmSelect = document.getElementById('algorithmSelect');
    const secretKey = document.getElementById('secretKey');
    const output = document.getElementById('encryptedOutput');
    const msgBox = document.getElementById('formMessage');
    const encryptBtn = document.getElementById('encryptBtn');

    encryptForm.addEventListener('submit', async (e) => {
        e.preventDefault();

        if (!messageInput.value.trim() || !secretKey.value.trim()) {
            showMessage(msgBox, 'Please fill in the message and secret key.', 'error');
            return;
        }

        setLoading(encryptBtn, true, 'Encrypt');
        try {
            const res = await fetch(`${API_BASE}/api/encrypt`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    message: messageInput.value,
                    algorithm: algorithmSelect.value,
                    key: secretKey.value,
                }),
            });
            const data = await res.json();
            if (!res.ok) throw new Error(data.error || 'Encryption failed.');

            output.value = data.result;
            showMessage(
                msgBox,
                data.hash
                    ? `Encrypted successfully. Integrity hash (SHA-256 via C++ engine): ${data.hash}`
                    : 'Encrypted successfully.',
                'success'
            );
        } catch (err) {
            showMessage(msgBox, err.message || 'Something went wrong. Is the server running?', 'error');
        } finally {
            setLoading(encryptBtn, false, 'Encrypt');
        }
    });

    encryptForm.addEventListener('reset', () => {
        output.value = '';
        showMessage(msgBox, '', '');
    });
}

// --------------------------------------------------------------------------
// DECRYPT PAGE
// --------------------------------------------------------------------------
const decryptForm = document.getElementById('decryptForm');
if (decryptForm) {
    const encryptedInput = document.getElementById('encryptedInput');
    const algorithmSelect = document.getElementById('algorithmSelect');
    const secretKey = document.getElementById('secretKey');
    const output = document.getElementById('decryptedOutput');
    const msgBox = document.getElementById('formMessage');
    const decryptBtn = document.getElementById('decryptBtn');

    decryptForm.addEventListener('submit', async (e) => {
        e.preventDefault();

        if (!encryptedInput.value.trim() || !secretKey.value.trim()) {
            showMessage(msgBox, 'Please paste the encrypted message and enter the secret key.', 'error');
            return;
        }

        setLoading(decryptBtn, true, 'Decrypt');
        try {
            const res = await fetch(`${API_BASE}/api/decrypt`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    message: encryptedInput.value.trim(),
                    algorithm: algorithmSelect.value,
                    key: secretKey.value,
                }),
            });
            const data = await res.json();
            if (!res.ok) throw new Error(data.error || 'Decryption failed.');

            output.value = data.result;
            showMessage(msgBox, 'Decrypted successfully.', 'success');
        } catch (err) {
            showMessage(msgBox, err.message || 'Decryption failed. Check your key and message.', 'error');
        } finally {
            setLoading(decryptBtn, false, 'Decrypt');
        }
    });

    decryptForm.addEventListener('reset', () => {
        output.value = '';
        showMessage(msgBox, '', '');
    });
}

// --------------------------------------------------------------------------
// LOGIN PAGE
// Demo-only client-side check (no database/user accounts exist in this
// project). Replace with a real authentication API before production use.
// --------------------------------------------------------------------------
const loginForm = document.getElementById('loginForm');
if (loginForm) {
    const username = document.getElementById('username');
    const password = document.getElementById('password');
    const msgBox = document.getElementById('formMessage');

    const DEMO_USER = 'admin';
    const DEMO_PASS = 'admin123';

    loginForm.addEventListener('submit', (e) => {
        e.preventDefault();

        if (!username.value.trim() || !password.value.trim()) {
            showMessage(msgBox, 'Please enter both username and password.', 'error');
            return;
        }

        if (username.value.trim() === DEMO_USER && password.value === DEMO_PASS) {
            sessionStorage.setItem('secureCryptLoggedIn', 'true');
            showMessage(msgBox, 'Login successful. Redirecting...', 'success');
            setTimeout(() => (window.location.href = 'index.html'), 700);
        } else {
            showMessage(msgBox, 'Invalid username or password. (Demo credentials: admin / admin123)', 'error');
        }
    });
}
