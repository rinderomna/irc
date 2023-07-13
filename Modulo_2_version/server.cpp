#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <vector>
#include <mutex>
#include <csignal>

using namespace std;

int serverSocket;
int clientIndex = 0;
mutex mtx;

struct Client {
    int socket;
    int index;
    string nickname;
    bool isAdmin;
    bool isMuted;
    string ip;
};

struct Channel{
    string name;
    vector<Client> clients;
};

vector<Channel> channels;
mutex clientsMtx;

void handleClient(int clientSocket, int index) {
    char buffer[4096] = {0};
    string message;
    bool connected = true;
    while (connected) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0)
            break;
        message = buffer;
        if (message == "/quit") {
            message = "Cliente " + to_string(index) + " se desconectou.";
            connected = false;
        } 
        else if (message == "/ping") {
            for (const auto &client : clients) {
                if (client.socket == clientSocket) {
                    string pong = "server: pong";
                    send(client.socket, pong.c_str(), pong.length(), 0);
                }
            }
        }
        else {
            message = to_string(index) + ": " + message;
            cout << message << endl;
            lock_guard<mutex> lock(clientsMtx);
            for (const auto &client : clients) {
                send(client.socket, message.c_str(), message.length(), 0);
            }
        }
    }
    close(clientSocket);
    lock_guard<mutex> lock(clientsMtx);

    for (int i = 0; i < (int)clients.size(); i++) {
        if(clients[i].socket == clientSocket){
            clients.erase(clients.begin() + i);
            cout << "Cliente " << index << " se desconectou." << endl;
            return;
        }
    }
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        for (const auto &client : clients) {
            close(client.socket);
            close(serverSocket);
        }
    }

    cout << "\nDesligando servidor..." << endl;

    exit(signal);
}

int main() {
    signal(SIGINT, signalHandler);

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, 3);

    cout << "Servidor iniciado. Aguardando conexões..." << endl;

    while (true) {
        int clientSocket;
        struct sockaddr_in clientAddress;
        socklen_t addrlen = sizeof(clientAddress);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addrlen);

        lock_guard<mutex> lock(mtx);
        clients.push_back({clientSocket, clientIndex++, ""});
        int currentIndex = clientIndex - 1;
        cout << "Cliente " << currentIndex << " se conectou!" << endl;

        thread clientThread(handleClient, clientSocket, currentIndex);
        clientThread.detach();
    }

    close(serverSocket);

    return 0;
}
