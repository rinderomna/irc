#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <csignal>

using namespace std;

// Variáveis Globais
int client_socket; // Socket do cliente

// Função para lidar com o recebimento de mensagens do servidor
void receiveMessages() {
    char buffer[4096] = {0};
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0)
            break;
        cout << '\t' << buffer << endl;
    }
}

// Função para lidar com o envio de mensagens ao servidor
void sendMessages() {
    string message;
    while (true) {
        getline(cin, message);
        send(client_socket, message.c_str(), message.length(), 0);
        if (message == "/quit")
            break;
    }
    close(client_socket);
}

// Função para lidar com interrupção do programa
void signalHandler(int signal) {
    if (signal == SIGINT) {
        string quit = "/quit";
        send(client_socket, quit.c_str(), quit.length(), 0);
        close(client_socket);
    }

    cout << "\nDesconectando..." << endl;

    exit(signal);
}

int main(int argc, char **argv) {
    signal(SIGINT, signalHandler);

    if (argc != 2) {
        cout << "use: ./client <IPaddress>\n";
        return -1;
    }

    // Criando o socket do cliente
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0) {
        cout << "Erro ao criar o socket." << endl;
        return -1;
    }

    // Configurando o socket do servidor com que o cliente irá se conectar
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    inet_pton(AF_INET, argv[1], &server_address.sin_addr);

    cout << "Digite /connect para se conectar ao servidor." << endl;

    string command;
    while (true) {
        cin >> command;
        
        // Comando para se conectar ao servidor
        if (command == "/connect") {
            if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
                cout << "Erro ao conectar ao servidor." << endl;
                return -1;
            }
            cout << "Conectado ao servidor." << endl;
            break;
        } 
        // Comando para desconectar
        else if (command == "/quit") {
            string quit = "/quit";
            send(client_socket, quit.c_str(), quit.length(), 0);
            close(client_socket);
            cout << "Programa encerrado." << endl;
            return 0;
        } 
        // Comando não é válido
        else {
            cout << "Comando inválido. Digite /connect para se conectar ao servidor." << endl;
        }
    }

    // Iniciando threads para lidar com recebimento e envio de mensagens
    thread receiveThread(receiveMessages);
    thread sendThread(sendMessages);

    receiveThread.join();
    sendThread.join();

    return 0;
}
