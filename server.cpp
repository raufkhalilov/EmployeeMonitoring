#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Функция для получения CLSID кодека
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;          // Количество кодеков
    UINT size = 0;        // Размер массива кодеков

    Gdiplus::ImageCodecInfo* pCodecInfo = nullptr;

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1; // Ошибка, если нет кодеков

    pCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    Gdiplus::GetImageEncoders(num, size, pCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pCodecInfo[j].Clsid;
            free(pCodecInfo);
            return j; // Возвращаем индекс
        }
    }
    free(pCodecInfo);
    return -1; // Кодек не найден
}

// Функция для захвата экрана
void captureScreen() {
    // Инициализация GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Захват экрана
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    SelectObject(hMemoryDC, hBitmap);
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

    // Сохранение изображения в файл
    CLSID clsid;
    GetEncoderClsid(L"image/png", &clsid);
    Gdiplus::Bitmap bmp(hBitmap, NULL);
    bmp.Save(L"saved_screenshot.png", &clsid, NULL); // Укажите путь для сохранения

    // Освобождение ресурсов
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
    GdiplusShutdown(gdiplusToken);
    std::cout << "Screen captured and saved to file." << std::endl;
}

// Функция для обработки клиента
void handleClient(SOCKET clientSocket) {
    char recvbuf[512];
    int recvbuflen = 512;
    int result;

    while (true) {
        result = recv(clientSocket, recvbuf, recvbuflen, 0);
        if (result > 0) {
            recvbuf[result] = '\0';  // Завершение строки
            std::cout << "Received command: " << recvbuf << std::endl;

            // Обработка команды
            if (strcmp(recvbuf, "capture") == 0) {
                captureScreen();
                std::string response = "Screenshot captured.";
                send(clientSocket, response.c_str(), (int)response.length(), 0);
            } else if (strncmp(recvbuf, "idle_time:", 10) == 0) {
                // Извлекаем время простоя
                std::string idleTime = recvbuf + 10; // После "idle_time:"
                std::cout << "Idle time received: " << idleTime << " seconds" << std::endl;
                std::string response = "Idle time recorded.";
                send(clientSocket, response.c_str(), (int)response.length(), 0);
            } else if (strcmp(recvbuf, "exit") == 0) {
                std::cout << "Client requested to close connection." << std::endl;
                break;
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

// Функция для отображения меню и обработки ввода
void menu(SOCKET clientSocket) {
    int choice;
    while (true) {
        std::cout << "Server Menu:" << std::endl;
        std::cout << "1. Capture Screen" << std::endl;
        std::cout << "2. Exit" << std::endl;
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                // Отправка команды захвата экрана клиенту
                send(clientSocket, "capture", strlen("capture"), 0);
                std::cout << "Capture command sent to client." << std::endl;
                break;
            case 2:
                send(clientSocket, "exit", strlen("exit"), 0);
                std::cout << "Exiting server." << std::endl;
                return; // Выход из меню
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

    // Инициализация WinSock
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Настройка адреса сервера
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

        // Создаем поток для обработки клиента
        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach(); // Отделяем поток

        // Вызываем меню для управления клиентом
        menu(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
