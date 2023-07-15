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

// Variávies Globais
int serverSocket;       // Socket do servidor
int clientIndex = 0;    // Índice do cliente para atribuição inicial de apelido
mutex mtx;              // Mutex para controle de acesso concorrente

// Declaração da estrutura que representa um canal de chat
struct Channel;

// Estrutura que representa um cliente conectado
struct Client {
    int socket;               // Socket do cliente
    int index;                // ìndice único do cliente
    string nickname;          // Apelido do cliente
    bool isAdmin;             // Indica se o cliente é um administrador do canal em que está
    bool isMuted;             // Indica se o cliente está silenciado
    Channel *currentChannel;  // Canal atual do cliente
    string ip;                // Endereço IP do cliente
};

// Definição da estrutura que representa um canal de chat
struct Channel{
    string name;                   // Nome do canal
    bool inviteMode;               // Indica se o canal é apenas para convidados
    vector<string> channelGuests;  // Lista de apelidos de convidados do canal
    vector<Client*> clients;       // Lista de clientes conectados ao canal
};

vector<Client*> clients;           // Lista de todos os clientes conectados
vector<Channel*> channels;         // Lista de todos os canais de chat
mutex clientsMtx;                  // Mutex para controle de acesso à lista de clientes

// Função auxiliar para dividir uma string em várias partes usando um separador específico
vector<string> customSplit(string str, char separator) {
    vector<string> strings;
    int startIndex = 0, endIndex = 0;

    // Itera sobre a string para encontrar o separador e dividir em partes
    for (int i = 0; i <= str.size(); i++) {
        if (str[i] == separator || i == str.size()) {
            endIndex = i;
            string temp = str.substr(startIndex, endIndex - startIndex);
            
            // Não permitir que string vazias sejam retornadas
            if (!temp.empty()) {
                strings.push_back(temp);
            }
            startIndex = endIndex + 1;
        }
    }
    return strings;
}

// Função para encontrar um cliente em um canal com base no apelido
Client *findClientInChannel(Channel *channel, string client) {
    for (const auto& c : channel->clients) {
        if (c->nickname == client) {
            return c;
        }
    }
    return nullptr;
}

// Função para encontrar um canal com base no nome
Channel *findChannel(string name) {
    for (const auto& c : channels) {
        if (c->name == name) {
            return c;
        }
    }
    return nullptr;
}

// Função para validar um apelido
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

// Função para verificar se um apelido já existe
bool existNickname(string nickname){
    for(const auto &c : clients){
        if(c->nickname == nickname){
            return true;
        }
    }
    return false;
}

// Função para remover um canal da lista de canais
void removeChannel(string channelName){
    for (auto it = channels.begin(); it != channels.end(); it++){
        if((*it)->name == channelName){
            channels.erase(it);
            delete *it;
            break;
        }
    }
}

// Função para remover um cliente do canal atual
// Remove o canal se o cliente for o último a sair
// Define o próximo administrador do canal
// Define o canal atual do cliente como nulo
void removeClient(Client* client) {
    if (!client->currentChannel) return;

    Channel* currentChannel = client->currentChannel;

    auto it = std::find_if(currentChannel->clients.begin(), currentChannel->clients.end(),
                           [&](const Client* c) { return c->nickname == client->nickname; });

    if (it != currentChannel->clients.end()) {
        // Se o cliente for o último no canal, remove o canal
        if (currentChannel->clients.size() == 1) {
            removeChannel(currentChannel->name);
        } else {
            // Avisar aos outros no canal que esta pessoa saiu
            string leave = "Server: O usuário " + client->nickname + " saiu do canal.";
            for (auto it2 = currentChannel->clients.begin(); it2 != currentChannel->clients.end(); it2++) {
                if ((*it2)->nickname != client->nickname) {
                    send((*it2)->socket, leave.c_str(), leave.length(), 0);
                }
            }

            // Se o cliente removido for um administrador, definir o próximo administrador do canal
            if ((*it)->isAdmin) {
                auto next = it + 1;
                if (next != currentChannel->clients.end()) {
                    (*next)->isAdmin = true;
                    string admin = "Server: Você se tornou o novo administrador do canal.";
                    send((*next)->socket, admin.c_str(), admin.length(), 0);
                }
            }

            // Remove o cliente do canal
            currentChannel->clients.erase(it);
        }
    }

    // Define o canal atual como nulo e garante que não está silenciado
    client->currentChannel = nullptr;
    client->isMuted = false;
}


// Função para lidar com a comunicação de um cliente
void handleClient(Client* client) {
    // Buffer para recepção de mensagens do cliente 
    char buffer[4096] = {0};
    string message;
    bool connected = true; // Booleano que controla se o cliente está conectado

    // Loop principal para receber mensagens do cliente
    while (connected) {
        // Limpar o buffer para receber a mensagem
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(client->socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) break;
        message = buffer;

        // Separar a mensagem recebida em tokens no espaços
        vector<string> tokens = customSplit(message, ' ');

        // Comando /quit para desconectar
        if (tokens[0] == "/quit") {
            connected = false;
        } 
        // Comando /ping => retornar 'pong'
        else if (tokens[0] == "/ping") {
            lock_guard<mutex> lock(clientsMtx);
            string pong = "Server: pong";
            send(client->socket, pong.c_str(), pong.length(), 0);
        }
        // Comando /nickname para trocar apelido
        else if (tokens[0] == "/nickname") {
            lock_guard<mutex> lock(clientsMtx);
            string nickname = "";

            // Parâmetro apelido foi passado
            if(tokens.size() > 1){
                // O apelido passado não existe ainda
                if(!existNickname(tokens[1])){
                    // O apelido passado é válido
                    if (validateNickname(tokens[1])) {
                        client->nickname = tokens[1];
                        nickname = "Server: Seu nickname foi alterado para " + client->nickname;
                    }
                    // O apelido passado é inválido
                    else {
                        nickname = "Server: Nome de usuário inválido. Não pode começar com dígito, não deve ter mais de 50 caracteres e não pode ser 'server'";
                    }
                }
                // O apelido passado já existe
                else {
                    nickname = "Server: Nome de usuário já existe, insira outro.";
                }
            } 
            // Parâmetro apelido não foi passado
            else {
                nickname = "Server: Para criar um nickname, digite /nickname <NICKNAME>";
            }

            send(client->socket, nickname.c_str(), nickname.length(), 0);
        }
        // Comando /join para adentrar um canal
        else if (tokens[0] == "/join") {
            lock_guard<mutex> lock(clientsMtx);

            // Parâmetro canal foi passado
            if(tokens.size() > 1) {
                Channel* channel;
                // O canal passado existe
                if((channel = findChannel(tokens[1]))){
                    // O canal é apenas para convidados
                    if (channel->inviteMode) {
                        // Buscar apelido do cliente na lista de convidados
                        auto it = find(
                            channel->channelGuests.begin(),
                            channel->channelGuests.end(),
                            client->nickname
                        );
                        // O cliente está na lista de convidados do canal passado
                        if (it != channel->channelGuests.end()) {
                            // Remover o cliente do canal em que está se estiver em algum já
                            if (client->currentChannel) {
                                removeClient(client);
                            }
                            message = "Server: Você adentrou o canal ";
                            message += "$";
                            message += channel->name;
                            message += " como convidado.";

                            string hello = "Server: O usuário " + client->nickname + " entrou no canal.";

                            // Iterar em todos os clientes para avisar que ele entrou
                            for (const auto& c : channel->clients) {
                                send(c->socket, hello.c_str(), hello.length(), 0);
                            }

                            // Adicionar cliente ao canal
                            channel->clients.push_back(client);
                            // Configurar o canal atual do cliente
                            client->currentChannel = channel;
                            // Configurar para não ser administrador 
                            client->isAdmin = false;
                        } 
                        // O cliente não está na lista de convidados do canal passado
                        else {
                            message = "Server: Um administrador deste canal deve te convidar para que você possa entrar.";
                        }
                    } 
                    // O canal passado é Livre
                    else {
                        // Remover o cliente do canal em que está se estiver em algum já
                        if (client->currentChannel) {
                            removeClient(client);
                        }

                        string hello = "Server: O usuário " + client->nickname + " entrou no canal.";

                        // Iterar em todos os clientes para avisar que ele entrou
                        for (const auto& c : channel->clients) {
                            send(c->socket, hello.c_str(), hello.length(), 0);
                        }

                        message = "Server: Você adentrou o canal ";
                        message += "#";
                        message += channel->name;

                        // Adicionar cliente ao canal
                        channel->clients.push_back(client);
                        // Configurar o canal atual do cliente
                        client->currentChannel = channel;
                        // Configurar para não ser administrador 
                        client->isAdmin = false;
                    }
                }
                // O canal passado ainda não existia
                else {
                    // Remover o cliente do canal em que está se estiver em algum já
                    if (client->currentChannel) {
                        removeClient(client);
                    }

                    // Criar um novo canal e colocar o cliente nele
                    channel = new Channel {
                        .name = tokens[1],
                        .inviteMode = false
                    };
                    client->isAdmin = true;
                    channel->clients.push_back(client);
                    channels.push_back(channel);

                    client->currentChannel = channel;
                    message = "Server: você criou o canal #";
                    message += channel->name;
                }

                send(client->socket, message.c_str(), message.length(), 0);
            }
            // Parâmetro canal não foi passado
            else {
                // Instruir como fazer /join e mostrar canais existentes
                message = "Server: faça /join <NOME_DO_CANAL>\n";
                message += "\tSe o canal não existir, ele é criado.\n";
                message += "\tCanais existentes:\n";

                for (int i = 0; i < (int)channels.size(); i++) {
                    message = message + "\t\t" + ((channels[i]->inviteMode) ? "$" : "#"); 
                    message = message + channels[i]->name + "\n";
                }

                if (channels.empty()) {
                    message += "\t\t[NÃO HÁ CANAIS EXISTENTES]";
                }

                send(client->socket, message.c_str(), message.length(), 0);
            }
        }
        // Comando /kick para expulsar um cliente do canal
        else if (tokens[0] == "/kick") {
            lock_guard<mutex> lock(clientsMtx);

            // O cliente que acionou o comando é um administrador do canal
            if (client->isAdmin) {
                // O parâmetro cliente a ser expulso foi passado
                if (tokens.size() > 1) {
                    string feedback = "";
                    Client *clientTarget = findClientInChannel(client->currentChannel, tokens[1]);

                    // Cliente passado existe no canal
                    if (clientTarget) {
                        feedback = " Você quem removeu " + clientTarget->nickname + " do canal.";
                        message = "Server: Você foi removido do canal pelo administrador.";
                        send(clientTarget->socket, message.c_str(), message.length(), 0);
                        removeClient(clientTarget);
                    } 
                    // Cliente passado não existe no canal
                    else {
                        feedback = "Server: O usuário " + tokens[1] + " não existe neste canal.";
                    }

                    send(client->socket, feedback.c_str(), feedback.length(), 0);
                } 
                // O parâmetro cliente a ser expulso não foi passado
                else {
                    message = "Server: Para remover usuário: /kick <NICKNAME>";
                    send(client->socket, message.c_str(), message.length(), 0);
                }
            }
            // O cliente que acionou o comando não é administrador do canal
            else {
                message = "Server: Você não tem permissão para remover um usuário deste canal.";
                send(client->socket, message.c_str(), message.length(), 0);
            }
        }
        // Comando /mute para silenciar um cliente no canal
        else if (tokens[0] == "/mute") {
            lock_guard<mutex> lock(clientsMtx);

            // O cliente que acionou o comando é um administrador do canal
            if (client->isAdmin) {
                // O parâmetro cliente a ser silenciado foi passado
                if (tokens.size() > 1) {
                    string feedback = "";
                    Client *clientTarget = findClientInChannel(client->currentChannel, tokens[1]);

                    // Cliente passado existe no canal
                    if (clientTarget) {
                        // Cliente passado não estava mutado
                        if (!clientTarget->isMuted) {
                            feedback = "Server: Você mutou " + clientTarget->nickname + ".";
                            clientTarget->isMuted = true;
                            message = "Server: Você foi mutado pelo administrador do canal.";
                            send(clientTarget->socket, message.c_str(), message.length(), 0);
                        } 
                        // Cliente passado já estava mutado
                        else {
                            feedback = "Server: O usuário " + tokens[1] + " já estava mutado no canal.";
                        }
                    } 
                    // Cliente passado não existe no canal
                    else {
                        feedback = "Server: O usuário " + tokens[1] + " não existe neste canal.";
                    }

                    send(client->socket, feedback.c_str(), feedback.length(), 0);
                } 
                // O parâmetro cliente a ser silenciado não foi passado
                else {
                    message = "Server: Para mutar usuário: /mute <NICKNAME>";
                    send(client->socket, message.c_str(), message.length(), 0);
                }
            }
            // O cliente que acionou o comando não é um administrador do canal
            else {
                message = "Server: Você não tem permissão para mutar um usuário deste canal.";
                send(client->socket, message.c_str(), message.length(), 0);
            }
        }
        // Comando /unmute para desmutar um cliente no canal
        else if (tokens[0] == "/unmute") {
            lock_guard<mutex> lock(clientsMtx);
            
            // O cliente que acionou o comando é um administrador do canal
            if (client->isAdmin) {
                // O parâmetro cliente a ser desmutado foi passado
                if (tokens.size() > 1) {
                    string feedback = "";
                    Client *clientTarget = findClientInChannel(client->currentChannel, tokens[1]);

                    // Cliente passado existe no canal
                    if (clientTarget) {
                        // Cliente passado estava mutado
                        if (clientTarget->isMuted) {
                            feedback = "Server: Você desmutou " + clientTarget->nickname + ".";
                            clientTarget->isMuted = false;
                            message = "Server: Você foi desmutado pelo administrador do canal.";
                            send(clientTarget->socket, message.c_str(), message.length(), 0);
                        } 
                        // Cliente passado já podia falar
                        else {
                            feedback = "Server: O usuário " + tokens[1] + " já estava desmutado no canal.";
                        }
                    } 
                    // Cliente passado não existe no canal
                    else {
                        feedback = "Server: O usuário " + tokens[1] + " não existe neste canal.";
                    }

                    send(client->socket, feedback.c_str(), feedback.length(), 0);
                } 
                // O parâmetro cliente a ser desmutado não foi passado
                else {
                    message = "Server: Para desmutar usuário: /unmute <NICKNAME>";
                    send(client->socket, message.c_str(), message.length(), 0);
                }
            }
            // O cliente que acionou o comando é um administrador do canal
            else {
                message = "Server: Você não tem permissão para desmutar um usuário deste canal.";
                send(client->socket, message.c_str(), message.length(), 0);
            }
        }
        // Comando /invite para adicionar apelido à lista de convidados do canal
        else if (tokens[0] == "/invite") {
            lock_guard<mutex> lock(clientsMtx);

            // O modo do canal é Livre
            if(!client->currentChannel->inviteMode){
                // O cliente que acionou o comando é um administrador
                if (client->isAdmin) {
                    message = "Server: O modo atual deste canal é Livre. para mudar, entre o comando /changemode.";
                }
                // O cliente que acionou o comando não é um administrador
                else {
                    message = "Server: Além deste canal ser Livre, você não é administrador dele.";
                }
            }
            // O modo do canal é 'Apenas Convidados' e o cliente que acionou o comando é um administrador deste canal
            else if (client->isAdmin) {
                // O parâmetro apelido a ser adicionado à lista de convidados foi passado
                if (tokens.size() > 1){
                    // O apelido passado é válido
                    if (validateNickname(tokens[1])) {
                        // Buscar apelido passado na lista de convidados
                        auto it = find(
                            client->currentChannel->channelGuests.begin(),
                            client->currentChannel->channelGuests.end(),
                            tokens[1]
                        );
                        // O apelido passado já estava na lista
                        if (it != client->currentChannel->channelGuests.end()) {
                            message = "Server: Este usuário já estava na lista de convidados do canal.";
                        }
                        // O apelido passado ainda não estava na lista
                        else {
                            // Adicionar o apelido passado à lista de convidados
                            client->currentChannel->channelGuests.push_back(tokens[1]);
                            message = "Server: Você adicionou o usuário " + tokens[1] + " à lista de convidados do canal.";
                        }
                    }
                    // O apelido passado não é válido
                    else {
                        message = "Server: Este nome de usuário é inválido.";
                    }
                } 
                // O parâmetro apelido a ser adicionado à lista de convidados não foi passado
                else {
                    message = "Server: Digite /invite <NICKNAME> para convidar um usuário.";
                }
            }
            // O modo do canal é 'Apenas Convidados' e mas o cliente que acionou o comando não é um administrador deste canal
            else {
                message = "Server: Você não tem permissão para convidar um usuário.";
            }
            send(client->socket, message.c_str(), message.length(), 0);
        }
        // Comando /changemode para trocar o modo do canal entre 'Livre' e 'Apenas Convidados'
        else if (tokens[0] == "/changemode") {
            lock_guard<mutex> lock(clientsMtx);

            // O cliente que acionou o comando é um administrador do canal
            if (client->isAdmin) {
                // O canal era apenas para convidados e passou a ser livre
                if (client->currentChannel->inviteMode) {
                    // Limpar lista de convidados
                    client->currentChannel->channelGuests.clear();
                }

                // Trocar modo do canal
                client->currentChannel->inviteMode = !client->currentChannel->inviteMode;
                string mode = client->currentChannel->inviteMode ? "Apenas Convidados." : "Livre.";
                message = "Server: Você alterou o modo do canal para " + mode;
            }
            // O cliente que acionou o comando não é um administrador do canal
            else {
                message = "Server: Você não tem permissão para mudar o modo de um canal.";
            }
            send(client->socket, message.c_str(), message.length(), 0);
        }
        // Comando /whois para perguntar pelo IP de um usuário
        else if (tokens[0] == "/whois") {
            lock_guard<mutex> lock(clientsMtx);
            
            // Cliente que acionou o comando é um administrador do canal
            if (client->isAdmin) {
                // O parâmetro cliente alvo foi passado
                if (tokens.size() > 1) {
                    Client *clientTarget = findClientInChannel(client->currentChannel, tokens[1]);

                    // Cliente passado existe no cananl
                    if (clientTarget) {
                        message = "Server: Ip do alvo é " + clientTarget->ip;
                        send(client->socket, message.c_str(), message.length(), 0);
                    } 
                    // Cliente passado não existe no canal
                    else {
                        message = "Server: O usuário " + tokens[1] + " não existe neste canal.";
                        send(client->socket, message.c_str(), message.length(), 0);
                    }
                } 
                // O parâmetro cliente alvo não foi passado
                else {
                    message = "Server: Para perguntar pelo ip de um usuário: /whois <NICKNAME>";
                    send(client->socket, message.c_str(), message.length(), 0);
                }
            } 
            // Cliente que acionou o comando não é um administrador do canal
            else {
                message = "Server: Você não tem permissão para perguntar pelo ip de um usuário deste canal.";
                send(client->socket, message.c_str(), message.length(), 0);
            }
        }
        // Mensagem do cliente não configura um comando
        else {
            lock_guard<mutex> lock(clientsMtx);

            // O não está atualmente em num canal
            if(!client->currentChannel){
                message = "Server: Você primeiro deve acessar um canal com /join <NOME_DO_CANAL>";
                send(client->socket, message.c_str(), message.length(), 0);
            }
            // O cliente está em um canal e não está mutado
            else if (!client->isMuted){
                string isAdmin = client->isAdmin ? "@" : "";
                string channelType = client->currentChannel->inviteMode ? "$" : "#";
                message = channelType + client->currentChannel->name + "/" + isAdmin + client->nickname + ": " + message;
                cout << message << endl;
                for (const auto& c : client->currentChannel->clients) {
                    send(c->socket, message.c_str(), message.length(), 0);
                }
            } 
            // O cliente está em um canal, mas está mutado
            else {
                message = "Server: Você não pode mandar mensagens neste canal, pois está mutado.";
                send(client->socket, message.c_str(), message.length(), 0);
            }
        }
    }

    // Lidar com a desconexão de um cliente
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
    // Encerra a conexão de todos os clientes e libera a memória
    for (const auto& client : clients) {
        close(client->socket);
        delete client;
    }

    // Libera a memória dos canais
    for (const auto& channel : channels) {
        delete channel;
    }

    // Encerra a conexão do servidor
    close(serverSocket);

    cout << "Encerrando o servidor..." << endl;

    exit(signal);
}

int main() {
    signal(SIGINT, signalHandler);

    // Cria um socket para o servidor
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == -1) {
        cerr << "Erro ao criar o socket do servidor" << endl;
        return -1;
    }

    // Configura o endereço e a porta do servidor
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    // Associa o socket ao endereço e à porta do servidor
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Erro ao associar o socket ao endereço e à porta do servidor" << endl;
        return -1;
    }
    
    // Coloca o socket do servidor em modo de escuta
    if (listen(serverSocket, SOMAXCONN) == -1) {
        cerr << "Erro ao colocar o socket do servidor em modo de escuta" << endl;
        return -1;
    }

    cout << "Servidor iniciado. Aguardando conexões..." << endl;

    // Loop principal para aceitar conexões de clientes
    while (true) {
        // Aceita uma nova conexão de cliente
        int clientSocket;
        struct sockaddr_in clientAddress;
        socklen_t addrlen = sizeof(clientAddress);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addrlen);

        if (clientSocket == -1) {
            cerr << "Erro ao aceitar a conexão do cliente" << endl;
            continue;
        }

        // Cria uma nova estrutura de cliente
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

        // Adicionar cliente à lista de clientes conectados
        clients.push_back(client);
        cout << "Cliente " << clientIndex++ << " se conectou!" << endl;
        
        // Cria uma nova t2hread para lidar com a comunicação do cliente
        thread clientThread(handleClient, client);
        clientThread.detach();
    }

    close(serverSocket);

    return 0;
}
