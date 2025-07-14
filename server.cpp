#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>

#pragma comment(lib, "ws2_32.lib")

void receiveMessages(SOCKET sock) {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) break;
        buffer[bytesReceived] = '\0';
        std::cout << buffer;
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5555);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
        std::cerr << "❌ Не вдалося підключитися до сервера\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::string nickname;
    std::cout << "Введіть ваше ім’я: ";
    std::getline(std::cin, nickname);

    send(clientSocket, nickname.c_str(), nickname.size(), 0);

    std::thread t(receiveMessages, clientSocket);
    t.detach();

    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message == "/exit") break;
        send(clientSocket, message.c_str(), message.size(), 0);
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
