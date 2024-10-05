#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>

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
    // DeleteObject(hBitmap);
    // DeleteDC(hMemoryDC);
    // ReleaseDC(NULL, hScreenDC);
    // GdiplusShutdown(gdiplusToken);
    std::cout << "Screen captured and saved to file." << std::endl;
}

void handleClient(SOCKET clientSocket) {
    char recvbuf[512];
    int recvbuflen = 512;
    int result;

    while (true) {
        // Ожидание команды от клиента
        result = recv(clientSocket, recvbuf, recvbuflen, 0);
        if (result > 0) {
            recvbuf[result] = '\0';  // Завершение строки
            std::cout << "Received command: " << recvbuf << std::endl;

            if (strcmp(recvbuf, "screenshot") == 0) {
                // Захват экрана
                captureScreen();
                std::string response = "Screenshot captured.";
                send(clientSocket, response.c_str(), (int)response.length(), 0);
            } else if (strcmp(recvbuf, "exit") == 0) {
                std::cout << "Client requested to close connection." << std::endl;
                break;
            } else {
                std::string response = "Unknown command.";
                send(clientSocket, response.c_str(), (int)response.length(), 0);
            }
        } else if (result == 0) {
            std::cout << "Connection closing..." << std::endl;
            break;
        } else {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // Закрытие соединения
    closesocket(clientSocket);
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
        handleClient(clientSocket);  // Обработка клиента в отдельной функции
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
