#include <iostream>
#include <string>
#include <cstring>

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

// Mock cryptography function. Replace this with your actual AES/RSA logic.
std::string processCrypto(const std::string& rawIncomingRequest) {
    std::cout << "[C++] Received Request: " << rawIncomingRequest << std::endl;

    // Minimal JSON Parser placeholder to identify the action
    if (rawIncomingRequest.find("\"action\":\"encrypt\"") != std::string::npos) {
        // Simple mock placeholder response. Replace with your AES-256 function output!
        return "{\"status\":\"success\", \"data\":\"[ENCRYPTED_MOCK_CIPHERTEXT]\"}";
    } 
    else if (rawIncomingRequest.find("\"action\":\"decrypt\"") != std::string::npos) {
        // Simple mock placeholder response. Replace with your Decryption function output!
        return "{\"status\":\"success\", \"data\":\"[DECRYPTED_MOCK_ORIGINAL_MESSAGE]\"}";
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

    std::cout << "🔒 [C++] Cryptography Core Listening on Port 8080...\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverFd, (struct sockaddr*)&clientAddr, &addrLen);
        
        if (clientSocket != INVALID_SOCKET) {
            char buffer[4096] = {0};
            // Read incoming string from the Node.js API Gateway
#ifdef _WIN32
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
#else
            int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
#endif

            if (bytesRead > 0) {
                std::string request(buffer);
                // Run your core cryptographic algorithms
                std::string response = processCrypto(request);
                
                // Write the result string back to the Node backend
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