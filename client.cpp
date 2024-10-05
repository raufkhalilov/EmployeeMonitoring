#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define DEFAULT_PORT "8888"

// Функция для получения времени простоя в секундах
int getIdleTime() {
    // Получаем время последней активности пользователя
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    GetLastInputInfo(&lii);

    // Вычисляем время простоя
    DWORD idleTime = (GetTickCount() - lii.dwTime) / 1000; // в секундах
    return static_cast<int>(idleTime);
}

// Функция для захвата экрана (по вашему выбору)
void captureScreen(SOCKET connectSocket) {
    std::string command = "capture";
    send(connectSocket, command.c_str(), (int)command.length(), 0);
    std::cout << "Screenshot request sent." << std::endl;

    // Ожидание ответа от сервера (например, подтверждение захвата экрана)
    char recvbuf[512];
    int result = recv(connectSocket, recvbuf, 512, 0);
    if (result > 0) {
        recvbuf[result] = '\0';
        std::cout << "Server response: " << recvbuf << std::endl;
    } else {
        std::cerr << "Failed to receive response from server." << std::endl;
    }
}

// Функция для отправки времени простоя на сервер
void sendIdleTime(SOCKET connectSocket) {
    while (true) {
        Sleep(5000); // Проверяем каждые 5 секунд
        int idleTime = getIdleTime(); // Получаем реальное время простоя

        std::string message = "idle_time:" + std::to_string(idleTime);
        send(connectSocket, message.c_str(), (int)message.length(), 0);
        std::cout << "Idle time sent to server: " << idleTime << " seconds." << std::endl;
    }
}

int main() {
    WSADATA wsaData;
    SOCKET connectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL, * ptr = NULL, hints;
    int iResult;

    // Инициализация WinSock
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Настройка адреса подключения
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Получение информации о сервере
    iResult = getaddrinfo(SERVER_IP, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    // Подключение к серверу
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return 1;
        }

        iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result); // Освобождение ресурсов

    if (connectSocket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server!" << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server!" << std::endl;

    // Обработка команд от сервера в отдельном потоке
    std::thread idleThread(sendIdleTime, connectSocket);
    idleThread.detach(); // Отделяем поток

    // Обработка команд от сервера
    char recvbuf[512];
    int recvbuflen = 512;
    while (true) {
        int result = recv(connectSocket, recvbuf, recvbuflen, 0);
        if (result > 0) {
            recvbuf[result] = '\0';
            std::cout << "Received command: " << recvbuf << std::endl;

            if (strcmp(recvbuf, "capture") == 0) {
                captureScreen(connectSocket);
            } else if (strcmp(recvbuf, "exit") == 0) {
                std::cout << "Server requested to close connection." << std::endl;
                break;
            }
        } else if (result == 0) {
            std::cout << "Connection closed." << std::endl;
            break;
        } else {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    closesocket(connectSocket);
    WSACleanup();

    return 0;
}
