//
// Created by Meibel on 18/3/2025.
//
#ifndef MPOINTER_H
#define MPOINTER_H

#include "SocketClient.h.cpp"

template <typename T>
class MPointer {
private:
    int id;
    static SocketClient* client;

public:
    static void Init() {
        client = new SocketClient();
    }

    static MPointer<T> New() {
        std::string response = client->sendMessage("Create");
        std::cout << "Respuesta del servidor en New(): '" << response << "'" << std::endl;

        if (response.empty() || response == "Error") {
            throw std::runtime_error("Error al recibir un ID valido del servidor.");
        }
        response.erase(response.find_last_not_of(" \n\r\t") + 1);

        int newId = std::stoi(response);
        return MPointer<T>(newId);
    }


    MPointer(int id) : id(id) {}

    void setValue(T value) {
        client->sendMessage("Set " + std::to_string(id) + " " + std::to_string(value));
    }

    T getValue() {
        std::string response = client->sendMessage("Get " + std::to_string(id));
        std::cout << "Respuesta del servidor en getValue(): " << response << std::endl;
        return static_cast<T>(std::stoi(response));
    }


    ~MPointer() {
        client->sendMessage("DecreaseRefCount " + std::to_string(id));
    }
};

template <typename T>
SocketClient* MPointer<T>::client = nullptr;

#endif // MPOINTER_H
