#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>
#include <fstream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

// Функция для обработки клиента
void handleClient(SOCKET clientSocket) {
    char recvbuf[512];
    int recvbuflen = 512;
    int result;

    while (true) {
        result = recv(clientSocket, recvbuf, recvbuflen, 0);
        if (result > 0) {
            recvbuf[result] = '\0';
            std::cout << "Received command: " << recvbuf << std::endl;

            if (strcmp(recvbuf, "idle_time") == 0) {
                std::cout << "Idle time received." << std::endl;
            } else if (strcmp(recvbuf, "exit") == 0) {
                std::cout << "Client requested to close connection." << std::endl;
                break;
            } else {
                // Получаем размер файла
                int fileSize;
                result = recv(clientSocket, (char*)&fileSize, sizeof(int), 0);
                if (result <= 0) {
                    std::cerr << "Failed to receive file size." << std::endl;
                    break;
                }

                std::vector<char> buffer(fileSize);
                // Получаем файл
                result = recv(clientSocket, buffer.data(), fileSize, 0);
                if (result <= 0) {
                    std::cerr << "Failed to receive screenshot." << std::endl;
                    break;
                }

                // Сохраняем скриншот в файл с новым именем
                std::ofstream outFile("temp_screenshot.png", std::ios::binary); // Изменено имя файла
                outFile.write(buffer.data(), fileSize);
                outFile.close();

                std::cout << "Screenshot received and saved." << std::endl;
            }
        } else if (result == 0) {
            std::cout << "Connection closing..." << std::endl;
            break;
        } else {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    closesocket(clientSocket);
}

void menu(SOCKET clientSocket) {
    int choice;
    while (true) {
        std::cout << "--------Server Menu--------" << std::endl;
        std::cout << "1. Request Screenshot" << std::endl;
        std::cout << "2. Exit" << std::endl;
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                if (send(clientSocket, "capture", strlen("capture"), 0) == SOCKET_ERROR) {
                    std::cerr << "Failed to send capture command." << std::endl;
                } else {
                    std::cout << "Capture command sent to client." << std::endl;
                }
                break;
            case 2:
                if (send(clientSocket, "exit", strlen("exit"), 0) == SOCKET_ERROR) {
                    std::cerr << "Failed to send exit command." << std::endl;
                } else {
                    std::cout << "Exiting server." << std::endl;
                }
                return;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
                break;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket;
    sockaddr_in serverAddr;

    WSAStartup(MAKEWORD(2, 2), &wsaData);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);
    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    std::cout << "Waiting for client connections..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::cout << "Client connected!" << std::endl;

        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();

        menu(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
