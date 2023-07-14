#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

int main() {
    /** Cria um socket para o client
     * // AF_INET diz que vamos usar IPv4
     * // SOCK_STREAM diz que vamos usar um stream confiável de bytes
     * // IPPROTO_TCP diz que vamos usar TCP. Poderíamos passar zero em 
     *   seu lugar, já que SOCK_STREAM por padrão usarái TCP.
    */
    int client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    /**
     * A struct sockaddr_in server_address vai armazenar as informações
     * do socket a que iremos nos conectar. No caso, nos conectaremos ao
     * localhost (127.0.0.1) na porta 8080 (o servidor deve estar escutando)
     * nesta porta.
     */
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_port = htons(8080); // Porta 8080
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr); // Associação com IP do servidor

    /**
     * Vamos agora de fato nos conectar ao servidor. Passamos os nosso socket que criamos (veja que não
     * especificamos em que porta estamos escutando, ou seja, não utilizamos bind, porque nós, como clientes
     * não nos importamos com isso. Temos nosso IP, independentemente de a partir de que porta, queremos nos)
     * conectar ao servidor num endereço IP e porta específica. No caso, sabemos que o servidor ao qual queremos
     * nos conector está rodando no endereço "127.0.0.1" escutando na porta 8080). Passamos em seguida as 
     * informações de endereço do socket do servidor, que configuramos acima. Por fim, o tamanho desta estrutura
     * do socket do servidor (por algum motivo, o C++ precisa disso). 
     */
    connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    /**
     * Uma vez que a conexão foi estabelecida a conexão, informamos isso com uma mensagem no terminal.
     */
    cout << "Você está conectado ao servidor :D" << endl;

    /**
     * Com a conexão estabelecida, 3-way handshake do TCP e tudo mais, podemos começar a troca de mensagens
     * através do uso das primitivas de socket 'send' e 'recv'.
     * 
     * Precisamos primeiro configurar o buffer que vai receber as mensagens com um tamanho arbitrário de 1024.
     * As mensagens vão sendo recebidas neste buffer, o qual podemos consumir para ler.
     */
    char buffer[1024] = {0};
    string mensagem;

    while (true) {
        // Ler entrada do usuário para mandar para o servidor.
        cout << "Escreva sua mensagem: ";
        getline(cin, mensagem);

        // Mandar mensagem ao servidor.
        send(client_socket, mensagem.c_str(), mensagem.length(), 0);

        // Checar se o usuário quer sair caso a mensagem enviada ao servidor tenha sido o comando '/exit'.
        if (mensagem == "/exit") break;

        // Receber resposta do servidor.
        recv(client_socket, buffer, 1024, 0);
        cout << "Mensagem do Servidor: " << buffer << endl;

        // Limpar buffer de recepção para se preparar para próxima mensagem
        memset(buffer, 0, sizeof(buffer));
    }

    // Fechar o socket
    close(client_socket);

    return 0;
}
