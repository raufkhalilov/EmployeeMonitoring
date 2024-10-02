#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <vector>
#include <string>

// Link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8080"

enum Command {
    TRACK_ACTIVITY,
    REQUEST_SCREENSHOT,
    INVALID_COMMAND
};

// Function to handle communication with each client in a separate thread
void handleClient(SOCKET ClientSocket) {
    char recvbuf[512];
    int recvbuflen = 512;
    int iResult;

    while (true) {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            recvbuf[iResult] = '\0'; // Null-terminate the string
            std::cout << "Received: " << recvbuf << std::endl;
        } else if (iResult == 0) {
            std::cout << "Connection closing..." << std::endl;
            break;
        } else {
            std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // Shutdown the connection
    shutdown(ClientSocket, SD_SEND);
    closesocket(ClientSocket);
}

void sendCommandToClient(SOCKET ClientSocket, Command cmd) {
    std::string message;
    switch (cmd) {
        case TRACK_ACTIVITY:
            message = "TRACK_ACTIVITY";
            break;
        case REQUEST_SCREENSHOT:
            message = "REQUEST_SCREENSHOT";
            break;
        default:
            message = "INVALID_COMMAND";
            break;
    }

    send(ClientSocket, message.c_str(), (int)message.length(), 0);
}

Command showMenu() {
    std::cout << "\nMenu:\n";
    std::cout << "1. Track Activity\n";
    std::cout << "2. Request Screenshot\n";
    std::cout << "Enter your choice: ";

    int choice;
    std::cin >> choice;

    switch (choice) {
        case 1:
            return TRACK_ACTIVITY;
        case 2:
            return REQUEST_SCREENSHOT;
        default:
            return INVALID_COMMAND;
    }
}

int main() {
    WSADATA wsaData;
    SOCKET ListenSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL, hints;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    // Set up the server socket
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port for the server
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    // Create a socket for listening
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        std::cerr << "Socket failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Bind the socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    // Start listening for incoming connections
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Waiting for client connections..." << std::endl;

    // Accept clients in a loop
    while (true) {
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::cout << "Client connected!" << std::endl;

        // Show the menu and send the selected command to the client
        Command cmd = showMenu();
        if (cmd != INVALID_COMMAND) {
            sendCommandToClient(ClientSocket, cmd);
        }

        // Handle each client in a separate thread for receiving data
        std::thread clientThread(handleClient, ClientSocket);
        clientThread.detach(); // Detach the thread to handle multiple clients simultaneously
    }

    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}
