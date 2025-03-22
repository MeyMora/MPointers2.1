#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <thread>
#include <sstream>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <mutex>
#include <cstring>
#include <vector> // para usar std::vector en el GC
#include <cstdlib> // para malloc y free

#pragma comment(lib, "Ws2_32.lib")

char* memoryBlock;
size_t memSizeBytes = 0;
std::string dumpFolder;

std::mutex memMutex;
std::unordered_map<int, int> refCount;  // ID → conteo de referencias

std::unordered_map<int, size_t> idToOffset;
int currentId = 1;

void dumpMemoryToFile() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::time_t t = system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    char filename[256];
    std::strftime(filename, sizeof(filename), "%Y-%m-%d_%H-%M-%S", &tm);
    std::ostringstream fullPath;
    fullPath << dumpFolder << "/" << filename << "-" << ms.count() << ".dump";

    std::ofstream dumpFile(fullPath.str());
    for (const auto& [id, offset] : idToOffset) {
        int value;
        std::memcpy(&value, memoryBlock + offset, sizeof(int));
        dumpFile << "ID: " << id << " → Value: " << value << "\n";
    }
    dumpFile.close();
}

std::string processRequest(const std::string& request) {
    std::istringstream ss(request);
    std::string command;
    ss >> command;

    if (command == "Create") {

        if (currentId * sizeof(int) >= memSizeBytes) return "Error: Memoria llena";
        std::string response;
        {
            std::lock_guard<std::mutex> lock(memMutex);
            response = processRequest(request);
        }

        int id = currentId++;
        size_t offset = (id - 1) * sizeof(int);
        idToOffset[id] = offset;
        std::memset(memoryBlock + offset, 0, sizeof(int));
        refCount[id] = 1;  // ← iniciar en 1

        return std::to_string(id);
    }


    if (command == "Set") {
        int id, value;
        ss >> id >> value;

        if (idToOffset.find(id) != idToOffset.end()) {
            std::memcpy(memoryBlock + idToOffset[id], &value, sizeof(int));
            dumpMemoryToFile();
            return "OK";
        }
        return "Error: ID no encontrado";
    }

    if (command == "Get") {
        int id;
        ss >> id;

        if (idToOffset.find(id) != idToOffset.end()) {
            int value;
            std::memcpy(&value, memoryBlock + idToOffset[id], sizeof(int));
            return std::to_string(value);
        }
        return "Error: ID no encontrado";
    }

    if (command == "DecreaseRefCount") {
        int id;
        ss >> id;

        if (refCount.find(id) != refCount.end()) {
            refCount[id]--;
            if (refCount[id] <= 0) {
                idToOffset.erase(id);
                refCount.erase(id);
                dumpMemoryToFile();
                return "Liberado";
            } else {
                return "OK";
            }
        }
        return "Error: ID no encontrado";
    }


    return "Comando no reconocido";

    if (command == "IncreaseRefCount") {
        int id;
        ss >> id;

        if (refCount.find(id) != refCount.end()) {
            refCount[id]++;
            return "OK";
        }
        return "Error: ID no encontrado";
    }
}




void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        std::string request(buffer);
        request.erase(request.find_last_not_of(" \n\r\t") + 1);

        std::string response = processRequest(request);

        send(clientSocket, response.c_str(), response.length(), 0);
    }

    closesocket(clientSocket);
}

void garbageCollectorLoop() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // cada 5 segundos

        std::lock_guard<std::mutex> lock(memMutex); // evitar condiciones de carrera

        std::vector<int> toDelete;
        for (const auto& [id, count] : refCount) {
            if (count <= 0) {
                toDelete.push_back(id);
            }
        }

        for (int id : toDelete) {
            idToOffset.erase(id);
            refCount.erase(id);
            std::cout << "[GC] Liberado ID " << id << " por tener 0 referencias" << std::endl;
        }

        if (!toDelete.empty()) {
            dumpMemoryToFile();
        }
    }
}




int main(int argc, char* argv[]) {
    if (argc != 7) {
        std::cerr << "Uso: ./mem-mgr --port PUERTO --memsize TAM_MB --dumpFolder CARPETA\n";
        return 1;
    }

    std::string port;
    for (int i = 1; i < argc; i += 2) {
        std::string arg = argv[i];
        if (arg == "--port") port = argv[i + 1];
        else if (arg == "--memsize") memSizeBytes = std::stoul(argv[i + 1]) * 1024 * 1024;
        else if (arg == "--dumpFolder") dumpFolder = argv[i + 1];
    }

    memoryBlock = (char*)malloc(memSizeBytes);
    if (!memoryBlock) {
        std::cerr << "Error al reservar memoria\n";
        return 1;
    }

    std::filesystem::create_directories(dumpFolder);

    memoryBlock = (char*)malloc(memSizeBytes);
    if (!memoryBlock) {
        std::cerr << "Error al reservar memoria\n";
        return 1;
    }
    // Iniciar GC
    std::thread(garbageCollectorLoop).detach();



    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    struct addrinfo hints = {}, *serverInfo;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, port.c_str(), &hints, &serverInfo);
    SOCKET serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
    bind(serverSocket, serverInfo->ai_addr, (int)serverInfo->ai_addrlen);
    listen(serverSocket, SOMAXCONN);

    std::cout << "Servidor escuchando en el puerto " << port << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        std::thread(handleClient, clientSocket).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    free(memoryBlock);
    return 0;
}