#define _WINSOCK_DEPRECATED_NO_WARNINGS   // щоб не сварився компілятор на старі функції типу inet_addr
#include <iostream>                       // для cin/cout
#include <winsock2.h>                     // бібліотека для сокетів на Windows
#include <ws2tcpip.h>                     // inet_pton, і т.д.
#include <thread>                         // для потоків (паралельно приймати повід.)
#include <string>                         // std::string

#pragma comment(lib, "ws2_32.lib")        // лінкуємо бібліотеку сокетів

// окрема функція, буде постійно читати з сервера і виводити на екран
void receiveMessages(SOCKET sock) {
    char buffer[1024];                    // буфер куди прилітають повідомлення
    while (true) {
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0); // читаємо з сервера
        if (bytesReceived <= 0) break;    // якщо помилка або сервер відключився
        buffer[bytesReceived] = '\0';     // ставимо нуль в кінець рядка
        std::cout << buffer;              // виводимо що отримали
    }
}

int main() {
    WSADATA wsaData;                      // спец. структура для ініціалізації WinSock
    WSAStartup(MAKEWORD(2, 2), &wsaData); // запускаємо WinSock 2.2

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0); // створюємо TCP сокет
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "❌ Сокет не створився\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};            // структура для адреси сервера
    serverAddr.sin_family = AF_INET;     // IPv4
    serverAddr.sin_port = htons(5555);   // порт (перетворений в network byte order)
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP-адреса сервера

    // Підключення до сервера
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
        std::cerr << "❌ Не вдалось підключитись до сервера\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // вводимо нік
    std::string nickname;
    std::cout << "Введи своє ім’я (нік): ";
    std::getline(std::cin, nickname);
    send(clientSocket, nickname.c_str(), nickname.size(), 0); // шлемо на сервер

    std::thread t(receiveMessages, clientSocket); // запускаємо паралельно потік
    t.detach(); // від’єднуємо, щоб сам працював

    // тепер в нескінченному циклі вводимо повідомлення
    std::string message;
    while (true) {
        std::getline(std::cin, message);         // читаємо з клави
        if (message == "/exit") break;           // вихід, якщо написав /exit
        send(clientSocket, message.c_str(), message.size(), 0); // відправляємо на сервер
    }

    // закриваєм все
    closesocket(clientSocket); 
    WSACleanup();
    return 0;
}
