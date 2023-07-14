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

struct Channel;

struct Client {
    int socket;
    int index;
    string nickname;
    bool isAdmin;
    bool isMuted;
    Channel *currentChannel;
    string ip;
};

struct Channel{
    string name;
    vector<Client*> clients;
};

vector<Client*> clients;
vector<Channel*> channels;
mutex clientsMtx;

vector<string> customSplit(string str, char separator) {
    vector <string> strings;
    int startIndex = 0, endIndex = 0;
    for (int i = 0; i <= str.size(); i++) {
        
        if (str[i] == separator || i == str.size()) {
            endIndex = i;
            string temp;
            //remover strings "" ""
            temp.append(str, startIndex, endIndex - startIndex);
            strings.push_back(temp);
            startIndex = endIndex + 1;
        }
    }
    return strings;
}

Client *findClientInChannel(Channel *channel, string client) {
    for (const auto& c : channel->clients) {
        if (c->name == client) {
            return c;
        }
    }
    return nullptr;
}

Channel *findChannel(string name) {
    for (const auto& c : channels) {
        if (c->name == name) {
            return c;
        }
    }
    return nullptr;
}

bool existNickname(string nickname){
    for(const auto &c : clients){
        if(c->nickname == nickname){
            return true;
        }
    }
    return false;
}

void handleClient( Client* client ) {
    char buffer[4096] = {0};
    string message;
    bool connected = true;
    while (connected) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(client->socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0)
            break;
        message = buffer;
        vector<string> tokens = customSplit(message, ' ');
        if (tokens[0] == "/quit") {
            connected = false;
        } 
        else if (tokens[0] == "/ping") {
            string pong = "server: pong";
            send(client->socket, pong.c_str(), pong.length(), 0);
        }
        // não pode existir dois usuarios com o mesmo nick
        else if (tokens[0] == "/nickname") {
            string nickname = "";
            if(tokens.size() > 1){
                if(!existNickname(tokens[1])){
                    client->nickname = tokens[1];
                    nickname = "Seu nickname foi alterado para " + client->nickname;
                }
                else {
                    nickname = "Nome de usuário já existe, insira outro";
                }
            } else {
                nickname = "Server: Para criar um nickname, digite /nickname <NICKNAME>";
            }
            send(client->socket, nickname.c_str(), nickname.length(), 0);
        }
        else if (tokens[0] == "/join") {
            if(tokens.size() > 1){
                Channel* channel;
                if((channel = findChannel(tokens[1]))){
                    cout << "encontrei um canal existente" << endl;
                    channel->clients.push_back(client);
                    client->isAdmin = false;
                }
                else {
                    channel = new Channel {
                        .name = tokens[1]
                    };
                    client->isAdmin = true;
                    channel->clients.push_back(client);
                    channels.push_back(channel);
                }
                client->currentChannel = channel;
            }
            else {
                // quero mandar todos os canais que existem e como criar um
            }
        }
        else {
            if(!client->currentChannel){
                message = "server: Você primeiro deve acessar um canal com /join <NOMEDOCANAL>";
                send(client->socket, message.c_str(), message.length(), 0);
            }
            else{
                lock_guard<mutex> lock(clientsMtx);
                message = "#" + client->currentChannel->name + "/" + client->nickname + ": " + message;
                cout << message << endl;
                for (const auto& c : client->currentChannel->clients) {
                    send(c->socket, message.c_str(), message.length(), 0);
                }
            }
        }
    }
    close(client->socket);
    lock_guard<mutex> lock(clientsMtx);

    cout << "Cliente " << client->nickname << " se desconectou." << endl;
    for (auto it = clients.begin(); it != clients.end(); ++it) {  
        if ((*it)->index == client->index) {
            delete *it;
            clients.erase(it);
            break;
        }  
    }
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        for (const auto& client : clients) {
            close(client->socket);
            delete client;
        }
        close(serverSocket);
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
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
        string clientIPAddress(clientIP);
        Client* client = new Client {
            .socket = clientSocket,
            .index = clientIndex,
            .nickname = to_string(clientIndex),
            .isAdmin = false,
            .isMuted = false,
            .currentChannel = nullptr,
            .ip = clientIPAddress,
        };
        clients.push_back(client);
        cout << "Cliente " << clientIndex++ << " se conectou!" << endl;
        thread clientThread(handleClient, client);
        clientThread.detach();
    }

    close(serverSocket);

    return 0;
}
