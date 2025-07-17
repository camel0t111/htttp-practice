#define _WINSOCK_DEPRECATED_NO_WARNINGS     // для inet_addr
#include <iostream>                         // для cout
#include <winsock2.h>                       // основне для WinSock
#include <ws2tcpip.h>                       // для inet_pton
#include <vector>                           // зберігаємо всі повідомлення в vector
#include <string>                           // string
#include <ctime>                            // щоб брати час
#include <thread>                           // кожен клієнт — окремий потік

#pragma comment(lib, "ws2_32.lib")          // підключаємо бібліотеку сокетів

// структура повідомлення: нік, текст, і час
struct Message {
    std::string nickname;
    std::string text;
    std::string timestamp;
};

std::vector<Message> messages;              // тут зберігаються всі повідомлення
CRITICAL_SECTION cs;                        // для безпеки при багатьох клієнтах

// повертає поточний час у вигляді рядка
std::string getCurrentTime() {
    time_t now = time(0);
    tm ltm;
    localtime_s(&ltm, &now);                   // Win версія localtime
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ltm); // красиво форматуєм
    return std::string(buf);
}

// шлемо всі повідомлення, які були збережені
void sendMessages(SOCKET clientSocket) {
    EnterCriticalSection(&cs);              // блокуємо доступ до messages
    for (const auto& msg : messages) {
        std::string line = "[" + msg.timestamp + "] " + msg.nickname + ": " + msg.text + "\n";
        send(clientSocket, line.c_str(), line.size(), 0);
    }
    send(clientSocket, "=== Кінець історії ===\n", 25, 0);
    LeaveCriticalSection(&cs);              // розблоковуємо
}

// обробка кожного клієнта окремо
void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0); // чекаємо нік
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }

    buffer[bytesReceived] = '\0';
    std::string nickname(buffer);           // зберігаємо нік

    sendMessages(clientSocket);             // шлемо історію

    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0); // читаємо повідомлення
        if (bytesReceived <= 0) break;
        buffer[bytesReceived] = '\0';

        std::string text(buffer);           // текст повідомлення
        std::string timestamp = getCurrentTime(); // час

        EnterCriticalSection(&cs);          // блокуємо messages
        messages.push_back({ nickname, text, timestamp }); // додаємо новий рядок
        LeaveCriticalSection(&cs);
    }

    closesocket(clientSocket);              // відключаємо клієнта
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);   // старт WinSock
    InitializeCriticalSection(&cs);         // ініціалізуємо mutex

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0); // створення сокета

    sockaddr_in serverAddr{};              // адреса сервера
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5555);     // порт 5555
    serverAddr.sin_addr.s_addr = INADDR_ANY; // приймаємо з будь-якої IP

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)); // прив'язка
    listen(serverSocket, SOMAXCONN);       // очікуємо на з'єднання

    std::cout << "[*] Сервер запущено на порті 5555...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL); // новий клієнт
        std::thread(handleClient, clientSocket).detach();       // запускаємо потік
    }

    DeleteCriticalSection(&cs);            // чистимо за собою
    closesocket(serverSocket);             // закриваємо сервер
    WSACleanup();                          // прибираємо все що запускали
    return 0;
}
