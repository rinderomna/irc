#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <vector>
#include <mutex>
#include <csignal>
#include <algorithm>
#include <string>

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
    vector<string> strings;
    int startIndex = 0, endIndex = 0;
    for (int i = 0; i <= str.size(); i++) {
        if (str[i] == separator || i == str.size()) {
            endIndex = i;
            string temp = str.substr(startIndex, endIndex - startIndex);
            if (!temp.empty()) {
                strings.push_back(temp);
            }
            startIndex = endIndex + 1;
        }
    }
    return strings;
}


Client *findClientInChannel(Channel *channel, string client) {
    for (const auto& c : channel->clients) {
        if (c->nickname == client) {
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

bool validateNickname(string& nickname) {
    // Verificar se o nickname começa com dígito
    if (isdigit(nickname[0])) {
        return false;
    }

    // Verificar se o nickname tem mais do que 50 caracteres
    if (nickname.length() > 50) {
        return false;
    }

    // Verificar se o nickname é igual à string "server" (ignorando o caso)
    string lowercaseNickname = nickname;
    transform(lowercaseNickname.begin(), lowercaseNickname.end(), lowercaseNickname.begin(), ::tolower);
    if (lowercaseNickname == "server") {
        return false;
    }

    return true;
}

bool existNickname(string nickname){
    for(const auto &c : clients){
        if(c->nickname == nickname){
            return true;
        }
    }
    return false;
}

void removeChannel(string channelName){
    for (auto it = channels.begin(); it != channels.end(); it++){
        if((*it)->name == channelName){
            channels.erase(it);
            delete *it;
            break;
        }
    }
}

//remove o cliente do canal atual se estiver em um
//remove o canal quando o cliente é o ultimo a sair
//Define o próximo administrador do canal
// Define o Canal atual do cliente para null
void removeClient(Client *client){
    if(!client->currentChannel) return;
    for (auto it = client->currentChannel->clients.begin(); it != client->currentChannel->clients.end(); it++) {  
        if ((*it)->nickname == client->nickname) {
            if(client->currentChannel->clients.size() == 1){
                //excluir canal
                removeChannel(client->currentChannel->name);
                break;
            }
            else if ((*it)->isAdmin){
                auto next = it + 1;
                (*next)->isAdmin = true;
            }
            client->currentChannel->clients.erase(it);
            break;
        }  
    }
    client->currentChannel = nullptr;
    client->isMuted = false;
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
            lock_guard<mutex> lock(clientsMtx);
            string pong = "server: pong";
            send(client->socket, pong.c_str(), pong.length(), 0);
        }
        else if (tokens[0] == "/nickname") {
            lock_guard<mutex> lock(clientsMtx);
            string nickname = "";
            if(tokens.size() > 1){
                if(!existNickname(tokens[1])){
                    if (validateNickname(tokens[1])) {
                        client->nickname = tokens[1];
                        nickname = "Server: Seu nickname foi alterado para " + client->nickname;
                    }
                    else {
                        nickname = "Server: Nome de usuário inválido. Não pode começar com dígito, não deve ter mais de 50 caracteres e não pode ser 'server'";
                    }
                }
                else {
                    nickname = "Server: Nome de usuário já existe, insira outro.";
                }
            } else {
                nickname = "Server: Para criar um nickname, digite /nickname <NICKNAME>";
            }
            send(client->socket, nickname.c_str(), nickname.length(), 0);
        }
        else if (tokens[0] == "/join") {
            lock_guard<mutex> lock(clientsMtx);
            if(tokens.size() > 1){
                Channel* channel;

                if (client->currentChannel) {
                    removeClient(client);
                }

                if((channel = findChannel(tokens[1]))){
                    channel->clients.push_back(client);
                    client->isAdmin = false;

                    message = "Server: você adentrou o canal ";
                    message += "#";
                    message += channel->name;
                }
                else {
                    channel = new Channel {
                        .name = tokens[1]
                    };
                    client->isAdmin = true;
                    channel->clients.push_back(client);
                    channels.push_back(channel);

                    message = "Server: você criou o canal #";
                    message += channel->name;
                }

                client->currentChannel = channel;
                send(client->socket, message.c_str(), message.length(), 0);
            }
            else {
                message = "Server: faça /join <NOME_DO_CANAL>\n";
                message += "\tSe o canal não existir, ele é criado.\n";
                message += "\tCanais existentes:\n";

                for (int i = 0; i < (int)channels.size(); i++) {
                    message += "\t\t* " + channels[i]->name + "\n";
                }

                if (channels.empty()) {
                    message += "\t\t[NÃO HÁ CANAIS EXISTENTES]";
                }

                send(client->socket, message.c_str(), message.length(), 0);
            }
        }
        else if (tokens[0] == "/kick") {
            lock_guard<mutex> lock(clientsMtx);
            if (client->isAdmin) {
                if (tokens.size() > 1) {
                    string feedback = "";
                    Client *clientTarget = findClientInChannel(client->currentChannel, tokens[1]);

                    if (clientTarget) {
                        feedback = "Server: Você removeu " + clientTarget->nickname + " do canal.";
                        message = "Server: Você foi removido do canal pelo administrador.";
                        send(clientTarget->socket, message.c_str(), message.length(), 0);
                        removeClient(clientTarget);
                    } else {
                        feedback = "Server: O usuário " + tokens[1] + " não existe neste canal.";
                    }

                    send(client->socket, feedback.c_str(), feedback.length(), 0);
                } else {
                    message = "Server: Para remover usuário: /kick <NICKNAME>";
                    send(client->socket, message.c_str(), message.length(), 0);
                }
            }
            else {
                message = "Server: Você não tem permissão para remover um usuário deste canal.";
                send(client->socket, message.c_str(), message.length(), 0);
            }
        }
        else if (tokens[0] == "/mute") {
            lock_guard<mutex> lock(clientsMtx);
            if (client->isAdmin) {
                if (tokens.size() > 1) {
                    string feedback = "";
                    Client *clientTarget = findClientInChannel(client->currentChannel, tokens[1]);

                    if (clientTarget) {
                        if (!clientTarget->isMuted) {
                            feedback = "Server: Você mutou " + clientTarget->nickname + ".";
                            clientTarget->isMuted = true;
                            message = "Server: Você foi mutado pelo administrador do canal.";
                            send(clientTarget->socket, message.c_str(), message.length(), 0);
                        } 
                        else {
                            feedback = "Server: O usuário " + tokens[1] + " já estava mutado no canal.";
                        }
                    } 
                    else {
                        feedback = "Server: O usuário " + tokens[1] + " não existe neste canal.";
                    }

                    send(client->socket, feedback.c_str(), feedback.length(), 0);
                } 
                else {
                    message = "Server: Para mutar usuário: /mute <NICKNAME>";
                    send(client->socket, message.c_str(), message.length(), 0);
                }
            }
            else {
                message = "Server: Você não tem permissão para mutar um usuário deste canal.";
                send(client->socket, message.c_str(), message.length(), 0);
            }
        }
        else if (tokens[0] == "/unmute") {
            lock_guard<mutex> lock(clientsMtx);
            if (client->isAdmin) {
                if (tokens.size() > 1) {
                    string feedback = "";
                    Client *clientTarget = findClientInChannel(client->currentChannel, tokens[1]);

                    if (clientTarget) {
                        if (clientTarget->isMuted) {
                            feedback = "Server: Você desmutou " + clientTarget->nickname + ".";
                            clientTarget->isMuted = false;
                            message = "Server: Você foi desmutado pelo administrador do canal.";
                            send(clientTarget->socket, message.c_str(), message.length(), 0);
                        } 
                        else {
                            feedback = "Server: O usuário " + tokens[1] + " já estava desmutado no canal.";
                        }
                    } 
                    else {
                        feedback = "Server: O usuário " + tokens[1] + " não existe neste canal.";
                    }

                    send(client->socket, feedback.c_str(), feedback.length(), 0);
                } 
                else {
                    message = "Server: Para desmutar usuário: /unmute <NICKNAME>";
                    send(client->socket, message.c_str(), message.length(), 0);
                }
            }
            else {
                message = "Server: Você não tem permissão para desmutar um usuário deste canal.";
                send(client->socket, message.c_str(), message.length(), 0);
            }
        }
        else if (tokens[0] == "/whois") {
            lock_guard<mutex> lock(clientsMtx);
            
            if (client->isAdmin) {
                if (tokens.size() > 1) {
                    Client *clientTarget = findClientInChannel(client->currentChannel, tokens[1]);

                    if (clientTarget) {
                        message = "Server: Ip do alvo é " + clientTarget->ip;
                        send(client->socket, message.c_str(), message.length(), 0);
                    } 
                    else {
                        message = "Server: O usuário " + tokens[1] + " não existe neste canal.";
                        send(client->socket, message.c_str(), message.length(), 0);
                    }
                } 
                else {
                    message = "Server: Para perguntar pelo ip de um usuário: /whois <NICKNAME>";
                    send(client->socket, message.c_str(), message.length(), 0);
                }
            } 
            else {
                message = "Server: Você não tem permissão para perguntar pelo ip de um usuário deste canal.";
                send(client->socket, message.c_str(), message.length(), 0);
            }
        }
        else {
            lock_guard<mutex> lock(clientsMtx);
            if(!client->currentChannel){
                message = "Server: Você primeiro deve acessar um canal com /join <NOME_DO_CANAL>";
                send(client->socket, message.c_str(), message.length(), 0);
            }
            else if (!client->isMuted){
                string isAdmin = client->isAdmin ? "@" : "";
                message = "#" + client->currentChannel->name + "/" + isAdmin + client->nickname + ": " + message;
                cout << message << endl;
                for (const auto& c : client->currentChannel->clients) {
                    send(c->socket, message.c_str(), message.length(), 0);
                }
            } else {
                message = "Server: Você não pode mandar mensagens neste canal, pois está mutado.";
                send(client->socket, message.c_str(), message.length(), 0);
            }
        }
    }

    lock_guard<mutex> lock(clientsMtx);

    close(client->socket);

    cout << "Cliente " << client->nickname << " se desconectou." << endl;
    removeClient(client);
    for (auto it = clients.begin(); it != clients.end(); it++) {  
        if ((*it)->nickname == client->nickname) {
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
