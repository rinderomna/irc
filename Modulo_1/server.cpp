#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

int main() {
    /** Cria um socket para o servidor
     * // AF_INET diz que vamos usar IPv4
     * // SOCK_STREAM diz que vamos usar um stream confiável de bytes
     * // IPPROTO_TCP diz que vamos usar TCP. Poderíamos passar zero em 
     *   seu lugar, já que SOCK_STREAM por padrão usarái TCP.
    */
    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    /**
     * A struct sockaddr_in server_address vai armazenar as informações
     * do socket que estamos criando para o servidor. No caso, vamos ser um
     * servidor que roda no localhost (127.0.0.1) e escuta na porta 8080.
     */
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);

    /**
     * Como somos um servidor, vamos utilizar a primitiva de socket 'bind' para 
     * atrelar nosso socket com as informações de nos endereço e porta específicos,
     * no caso porta '8080'. 
     */
    bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    /**
     * Vamos agora explicitamente escutar e aguardar por conexões.
     * O 3 indica conseguimos receber até 3 conexões que ficam aguardando em uma fila.
     */
    listen(server_socket, 3);

    /**
     * Acima dissemos que vamos escutar por conexões. Agora precisamos aceitar estas 
     * conexões para poder lidar com cada uma delas.
     */
    int client_socket;
    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(client_address);
    client_socket = accept(server_socket, (struct sockaddr *)&client_address, &addrlen);

    /**
     * Estabelecida a conexão, podemos informar isto pelo terminal. 
     */
    cout << "Um cliente está conectado com você :D" << endl;

    /**
     * Com a conexão estabelecida, 3-way handshake do TCP e tudo mais, podemos começar a troca de mensagens
     * através do uso das primitivas de socket 'send' e 'recv'.
     * 
     * Precisamos primeiro configurar o buffer que vai receber as mensagens com um tamanho arbitrário de 1024.
     * As mensagens vão sendo recebidas neste buffer, o qual podemos consumir para ler.
     */
    char buffer[1024] = {0};
    string message;

    while (true) {
        // Primeiro recebemos a mensagem que o cliente nos enviou.
        recv(client_socket, buffer, 1024, 0);
        cout << "Mensagem do Cliente: " << buffer << endl;

        // Lida e impressa a mensagem, podemos limpar o buffer para preparar para novas mensagens.
        memset(buffer, 0, sizeof(buffer));

        // Checar se o cliente quer fechar a conexão com comando '/exit'.
        if (message == "/exit") break;

        // Ler entrada do usuário para mandar para o cliente.
        cout << "Escreva sua mensagem: ";
        getline(cin, message);

        // Mandar mensagem para o cliente.
        send(client_socket, message.c_str(), message.length(), 0);
    }

    // Fechar os sockets
    close(client_socket);
    close(server_socket);

    return 0;
}
