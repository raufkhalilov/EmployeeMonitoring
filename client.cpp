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

// Флаг для управления выходом из функции sendIdleTime
bool stopIdleMonitoring = false;

void captureScreen(SOCKET connectSocket) {
    std::string command = "screenshot";
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

void sendIdleTime(SOCKET connectSocket) {
    stopIdleMonitoring = false; // Сброс флага перед началом мониторинга

    while (!stopIdleMonitoring) {
        LASTINPUTINFO lii = { 0 };
        lii.cbSize = sizeof(LASTINPUTINFO);

        if (GetLastInputInfo(&lii)) {
            DWORD currentTime = GetTickCount();
            DWORD idleTime = (currentTime - lii.dwTime) / 1000;

            std::string message = "idle:" + std::to_string(idleTime);
            std::cout << "Sending: " << message << std::endl;

            // Отправка сообщения на сервер
            int iResult = send(connectSocket, message.c_str(), (int)message.length(), 0);
            if (iResult == SOCKET_ERROR) {
                std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
                break;
            }
        }

        // Ожидание 5 секунд перед отправкой снова
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    std::cout << "Stopped monitoring idle time." << std::endl;
}

int main() {
    WSADATA wsaData;
    SOCKET connectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL, * ptr = NULL, hints;
    int iResult;

    // Инициализация WinSock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    // Настройка соединения
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Разрешение адреса сервера
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
        break;
    }

    freeaddrinfo(result);

    if (connectSocket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server!" << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server!" << std::endl;

    while (true) {
        std::cout << "\nChoose an action:\n";
        std::cout << "1. Capture Screen\n";
        std::cout << "2. Monitor Idle Time\n";
        std::cout << "3. Exit\n";
        std::cout << "Enter your choice: ";
        
        int choice;
        std::cin >> choice;

        if (choice == 1) {
            captureScreen(connectSocket);
        } else if (choice == 2) {
            std::cout << "Monitoring idle time. Press Ctrl+C to stop." << std::endl;

            // Запуск мониторинга в отдельном потоке
            std::thread idleMonitor(sendIdleTime, connectSocket);
            
            // Ожидание завершения мониторинга
            std::cout << "Press Enter to stop monitoring idle time.\n";
            std::cin.ignore(); // Игнорирование остаточного '\n' от предыдущего ввода
            std::cin.get();    // Ожидание нажатия Enter

            // Завершение мониторинга
            stopIdleMonitoring = true;
            idleMonitor.join();
        } else if (choice == 3) {
            std::cout << "Exiting the program." << std::endl;
            break; // Выход из программы
        } else {
            std::cout << "Invalid choice, please try again." << std::endl;
        }
    }

    // Корректное завершение соединения
    shutdown(connectSocket, SD_SEND);
    closesocket(connectSocket);
    WSACleanup();

    return 0;
}
