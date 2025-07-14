#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <ctime>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

struct Message {
    std::string nickname;
    std::string text;
    std::string timestamp;
};

std::vector<Message> messages;
CRITICAL_SECTION cs;

std::string getCurrentTime() {
    time_t now = time(0);
    tm ltm;
    localtime_s(&ltm, &now);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ltm);
    return std::string(buf);
}

void sendMessages(SOCKET clientSocket) {
    EnterCriticalSection(&cs);
    for (const auto& msg : messages) {
        std::string line = "[" + msg.timestamp + "] " + msg.nickname + ": " + msg.text + "\n";
        send(clientSocket, line.c_str(), line.size(), 0);
    }
    send(clientSocket, "=== Кінець історії ===\n", 25, 0);
    LeaveCriticalSection(&cs);
}

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }

    buffer[bytesReceived] = '\0';
    std::string nickname(buffer);

    sendMessages(clientSocket);

    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) break;

        buffer[bytesReceived] = '\0';
        std::string text(buffer);
        std::string timestamp = getCurrentTime();

        EnterCriticalSection(&cs);
        messages.push_back({ nickname, text, timestamp });
        LeaveCriticalSection(&cs);
    }

    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    InitializeCriticalSection(&cs);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5555);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);
    std::cout << "[*] Сервер запущено на порті 5555...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        std::thread(handleClient, clientSocket).detach();
    }

    DeleteCriticalSection(&cs);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
