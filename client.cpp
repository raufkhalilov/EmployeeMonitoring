#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    sockaddr_in serverAddr;

    // Инициализация WinSock
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Настройка адреса сервера
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Подключение к серверу
    connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    std::cout << "Screenshot request sent." << std::endl;

    // Закрытие сокета
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
