// server.cpp — SecureCrypt Cryptography Core
//
// Listens on TCP port 8080 for requests from the Node.js gateway (server.js)
// and computes a real SHA-256 integrity hash for the "hash" action. This is
// the piece of the stack that demonstrates raw socket programming + SHA-256
// hashing described on the About page. AES encryption/decryption itself is
// handled in server.js using Node's audited crypto module.

#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <vector>
#include <sstream>
#include <iomanip>

// Platform-specific networking libraries (Supports Windows & Linux/macOS)
#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    typedef int SOCKET;
    const int INVALID_SOCKET = -1;
    const int SOCKET_ERROR = -1;
#endif

// ============================================================================
// SHA-256 implementation (FIPS 180-4), self-contained, no external libraries.
// ============================================================================
namespace sha256 {

static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

std::string hash(const std::string& input) {
    uint32_t h[8] = {
        0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
    };

    // Pre-processing: padding
    std::vector<uint8_t> msg(input.begin(), input.end());
    uint64_t bitLen = (uint64_t)msg.size() * 8;
    msg.push_back(0x80);
    while (msg.size() % 64 != 56) msg.push_back(0x00);
    for (int i = 7; i >= 0; i--) msg.push_back((uint8_t)((bitLen >> (i * 8)) & 0xff));

    // Process each 512-bit chunk
    for (size_t chunkStart = 0; chunkStart < msg.size(); chunkStart += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            w[i] = ((uint32_t)msg[chunkStart + i * 4] << 24) |
                   ((uint32_t)msg[chunkStart + i * 4 + 1] << 16) |
                   ((uint32_t)msg[chunkStart + i * 4 + 2] << 8) |
                   ((uint32_t)msg[chunkStart + i * 4 + 3]);
        }
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = rotr(w[i-15], 7) ^ rotr(w[i-15], 18) ^ (w[i-15] >> 3);
            uint32_t s1 = rotr(w[i-2], 17) ^ rotr(w[i-2], 19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }

        uint32_t a=h[0], b=h[1], c=h[2], d=h[3], e=h[4], f=h[5], g=h[6], hh=h[7];

        for (int i = 0; i < 64; i++) {
            uint32_t S1 = rotr(e,6) ^ rotr(e,11) ^ rotr(e,25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t temp1 = hh + S1 + ch + K[i] + w[i];
            uint32_t S0 = rotr(a,2) ^ rotr(a,13) ^ rotr(a,22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;

            hh = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }

        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }

    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (int i = 0; i < 8; i++) out << std::setw(8) << h[i];
    return out.str();
}

} // namespace sha256

// ============================================================================
// Minimal helper to pull a string field's value out of a flat JSON object.
// Sufficient for the simple {"action":"...","data":"..."} payloads sent by
// the Node.js gateway (data is base64 + ':' only, so no embedded quotes).
// ============================================================================
std::string extractJsonField(const std::string& json, const std::string& field) {
    std::string key = "\"" + field + "\":\"";
    size_t start = json.find(key);
    if (start == std::string::npos) return "";
    start += key.size();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    return json.substr(start, end - start);
}

std::string processCrypto(const std::string& rawIncomingRequest) {
    std::cout << "[C++] Received Request: " << rawIncomingRequest << std::endl;

    if (rawIncomingRequest.find("\"action\":\"hash\"") != std::string::npos) {
        std::string data = extractJsonField(rawIncomingRequest, "data");
        std::string digest = sha256::hash(data);
        return "{\"status\":\"success\", \"data\":\"" + digest + "\"}";
    }

    return "{\"status\":\"error\", \"message\":\"Unknown operation\"}";
}

int main() {
    // Initialize Windows Sockets if compiling on Windows
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock initialization failed.\n";
        return 1;
    }
#endif

    SOCKET serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        return 1;
    }

    int opt = 1;
#ifndef _WIN32
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080); // Listens internally on port 8080

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Socket Bind failed.\n";
        return 1;
    }

    if (listen(serverFd, 3) == SOCKET_ERROR) {
        std::cerr << "Listen loop failed.\n";
        return 1;
    }

    std::cout << "[C++] Cryptography Core (SHA-256 integrity engine) listening on port 8080...\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverFd, (struct sockaddr*)&clientAddr, &addrLen);

        if (clientSocket != INVALID_SOCKET) {
            char buffer[8192] = {0};
            // Read incoming string from the Node.js API Gateway
#ifdef _WIN32
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
#else
            int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
#endif

            if (bytesRead > 0) {
                std::string request(buffer, bytesRead);
                std::string response = processCrypto(request);
                send(clientSocket, response.c_str(), response.length(), 0);
            }
#ifdef _WIN32
            closesocket(clientSocket);
#else
            close(clientSocket);
#endif
        }
    }

#ifdef _WIN32
    closesocket(serverFd);
    WSACleanup();
#else
    close(serverFd);
#endif
    return 0;
}
