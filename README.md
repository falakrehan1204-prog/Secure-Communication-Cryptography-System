# SecureCrypt — Setup Guide (Beginner Friendly)

Follow these steps in order. Don't skip any step.

## Step 1: Open the project folder in a terminal

1. Extract this zip file somewhere on your computer (e.g. Desktop)
2. Open the extracted folder in **VS Code**
3. In VS Code, go to menu **Terminal → New Terminal** (this opens a terminal
   already pointed at your project folder — easier than using cmd separately)

## Step 2: Install the required packages (only once)

In the VS Code terminal, type:

```
npm install
```

Press Enter and wait. This downloads `express` and `cors` (the two packages
`server.js` needs) into a new `node_modules` folder. You only need to do
this once — not every time you run the project.

## Step 3: Start the Node.js server

Still in the terminal, type:

```
node server.js
```

You should see:

```
🚀 SecureCrypt Gateway running at http://localhost:3000
```

**Keep this terminal open** — closing it stops the server.

## Step 4: Open the website

Open your browser and go to:

```
http://localhost:3000
```

⚠️ Important: Do **NOT** use the "Live Server" extension (127.0.0.1:5500) —
open it through **http://localhost:3000** instead, since that's the address
your own `server.js` serves the site from. Encrypt/Decrypt/Login will now
all work — not just the home page.

Try it:
- Go to **Encrypt**, type a message + secret key, click **Encrypt**
- Copy the output, go to **Decrypt**, paste it + the same secret key, click
  **Decrypt** — you should get your original message back
- Go to **Login**, use username `admin` and password `admin123`

## Step 5 (optional but recommended): Run the C++ integrity engine

This step is optional — encryption/decryption already work without it. But
if you want the "SHA-256 integrity hash" message to appear after encrypting
(and to actually use your C++ code as intended), do this in a **second**
terminal (keep the first one running `node server.js`):

1. Open a **new terminal** in VS Code (don't close the first one)
2. Compile the C++ file:
   ```
   g++ -std=c++17 -o server_cpp server.cpp
   ```
3. Run it:
   ```
   ./server_cpp
   ```
   (On Windows, if that doesn't work, try `server_cpp.exe`)
4. You should see:
   ```
   [C++] Cryptography Core (SHA-256 integrity engine) listening on port 8080...
   ```

Now when you encrypt a message, the C++ program will print the request it
received, and the website will show a SHA-256 integrity hash next to your
encrypted result.

## What each file does

| File | Purpose |
|---|---|
| `index.html`, `about.html`, `encrypt.html`, `decrypt.html`, `login.html` | The web pages |
| `style.css` | All the styling |
| `script.js` | Makes the buttons/forms actually work (talks to the server) |
| `server.js` | Node.js backend — does the real AES-256 encryption/decryption |
| `server.cpp` | C++ backend — computes a SHA-256 integrity hash over a TCP socket (optional, Step 5) |
| `package.json` | Tells `npm install` which packages to download |

## Notes / honest limitations

- **Login is demo-only.** There's no real user database — it just checks
  for the hardcoded `admin` / `admin123` in the browser. Good enough for a
  college project demo, but don't use this pattern for anything real.
- **RSA** is mentioned on the About/Home pages as part of the tech stack
  description, but there's no actual RSA key-exchange feature wired into
  the UI (the form only offers AES-256/192/128). If your project
  requirements need a working RSA feature, let me know and I'll add it.
