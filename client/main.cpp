#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>

// Link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8080"
#define SERVER_IP "127.0.0.1" // Server IP (localhost for now)

// Function to capture a screenshot and save it to a file
void captureScreenshot(const std::string& filename) {
    // Get the device context of the screen
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    int width = GetDeviceCaps(hScreenDC, HORZRES);
    int height = GetDeviceCaps(hScreenDC, VERTRES);

    // Create a compatible bitmap from the screen DC
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);

    // Select the new bitmap into the memory DC
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    // Bit blit from the screen DC to the memory DC
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

    // Restore the old bitmap
    SelectObject(hMemoryDC, hOldBitmap);

    // Save the bitmap to a file (in BMP format)
    BITMAP bmp;
    PBITMAPINFO pbmi;
    WORD cClrBits;
    HANDLE hf;                 // file handle
    BITMAPFILEHEADER hdr;       // bitmap file-header
    PBITMAPINFOHEADER pbih;     // bitmap info-header
    LPBYTE lpBits;              // memory pointer
    DWORD dwTotal;              // total count of bytes
    DWORD cb;                   // incremental count of bytes
    BYTE *hp;                   // byte pointer

    GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bmp);
    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
    cClrBits = cClrBits <= 1 ? 1 : cClrBits <= 4 ? 4 : cClrBits <= 8 ? 8 : 24;
    pbmi = (PBITMAPINFO) LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * ((cClrBits < 24) ? (1 << cClrBits) : 0));

    pbih = (PBITMAPINFOHEADER) pbmi;
    pbih->biSize = sizeof(BITMAPINFOHEADER);
    pbih->biWidth = bmp.bmWidth;
    pbih->biHeight = bmp.bmHeight;
    pbih->biPlanes = bmp.bmPlanes;
    pbih->biBitCount = bmp.bmBitsPixel;
    if (cClrBits < 24) pbih->biClrUsed = (1 << cClrBits);
    pbih->biCompression = BI_RGB;
    pbih->biSizeImage = ((bmp.bmWidth * cClrBits +31) & ~31) /8 * bmp.bmHeight;
    pbih->biClrImportant = 0;

    hf = CreateFile(filename.c_str(), GENERIC_READ | GENERIC_WRITE, (DWORD) 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);
    hdr.bfType = 0x4d42;
    hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof (RGBQUAD) + pbih->biSizeImage);
    hdr.bfReserved1 = 0;
    hdr.bfReserved2 = 0;
    hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof (RGBQUAD);

    WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), (LPDWORD) &dwTotal, NULL);
    WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof (RGBQUAD), (LPDWORD) &dwTotal, NULL);

    lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);
    GetDIBits(hMemoryDC, hBitmap, 0, (WORD) bmp.bmHeight, lpBits, pbmi, DIB_RGB_COLORS);
    WriteFile(hf, (LPVOID) lpBits, (int) pbih->biSizeImage, (LPDWORD) &dwTotal, NULL);

    GlobalFree((HGLOBAL)lpBits);
    CloseHandle(hf);

    // Release the DC and free up memory
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
    DeleteObject(hBitmap);
}

// Function to send a screenshot file to the server
void sendScreenshot(SOCKET ConnectSocket, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (file) {
        std::string fileContents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // Send file size first
        int fileSize = (int)fileContents.size();
        send(ConnectSocket, (const char*)&fileSize, sizeof(int), 0);

        // Send file data
        send(ConnectSocket, fileContents.c_str(), fileSize, 0);
    } else {
        std::cerr << "Failed to open screenshot file!" << std::endl;
    }
}

int main() {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL, * ptr = NULL, hints;
    int iResult;
    std::string message;
    char recvbuf[512];
    int recvbuflen = 512;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    // Set up the connection details
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(SERVER_IP, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    // Attempt to connect to the server
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return 1;
        }

        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server!" << std::endl;
        WSACleanup();
        return 1;
    }

    // Main loop to receive commands from server
    while (true) {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            recvbuf[iResult] = '\0'; // Null-terminate the string
            std::string command = recvbuf;

            if (command == "TRACK_ACTIVITY") {
                std::cout << "Tracking activity..." << std::endl;
                // Code for tracking idle time
                LASTINPUTINFO lii = { 0 };
                lii.cbSize = sizeof(LASTINPUTINFO);

                if (GetLastInputInfo(&lii)) {
                    DWORD currentTime = GetTickCount();
                    DWORD idleTime = (currentTime - lii.dwTime) / 1000;

                    message = "IdleTime: " + std::to_string(idleTime);
                    send(ConnectSocket, message.c_str(), (int)message.length(), 0);
                }

            } else if (command == "REQUEST_SCREENSHOT") {
                std::cout << "Capturing and sending screenshot..." << std::endl;
                captureScreenshot("screenshot.bmp");
                sendScreenshot(ConnectSocket, "screenshot.bmp");
            }
        } else if (iResult == 0) {
            std::cout << "Connection closed by server." << std::endl;
            break;
        } else {
            std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // Shutdown connection
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "Shutdown failed: " << WSAGetLastError() << std::endl;
    }

    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
