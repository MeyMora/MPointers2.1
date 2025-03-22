//
// Created by Meibel on 18/3/2025.
//
#ifndef SOCKETCLIENT_H
#define SOCKETCLIENT_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER "127.0.0.1"
#define PORT "8080"

class SocketClient {
private:
    SOCKET clientSocket;

public:
    SocketClient() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        struct addrinfo hints = {}, *serverInfo;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        getaddrinfo(SERVER, PORT, &hints, &serverInfo);
        clientSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
        if (connect(clientSocket, serverInfo->ai_addr, (int)serverInfo->ai_addrlen) == SOCKET_ERROR) {
            std::cerr << "Error al conectar al servidor." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            exit(1);
        }


        connect(clientSocket, serverInfo->ai_addr, (int)serverInfo->ai_addrlen);
        freeaddrinfo(serverInfo);
    }

    std::string sendMessage(const std::string& msg) {
        std::cout << "Enviando al servidor: '" << msg << "'" << std::endl;

        send(clientSocket, msg.c_str(), msg.length(), 0);

        char buffer[1024] = {0};
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0) {
            return "Error"; // Si no recibe nada, retorna un error
        }

        buffer[bytesReceived] = '\0';
        std::string response(buffer);

        std::cout << "Respuesta cruda del servidor: '" << response << "'" << std::endl;
        return response;
    }

    ~SocketClient() {
        closesocket(clientSocket);
        WSACleanup();
    }
};

#endif // SOCKETCLIENT_H
