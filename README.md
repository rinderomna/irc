# Trabalho Camadas de Transporte e Aplicação - protocolo IRC

Repositório para o Trabalho 2 da disciplina "SSC0142 - Redes de Computadores" do ICMC - USP - São Carlos, ministrada pela professora Kalinka Regina Lucas Jaquie Castelo Branco.

[VÍDEO FINAL](https://www.youtube.com/watch?v=-HIGsdjxWw0)

Informações do Sistema Operacional em que o programa foi testado:
Distributor ID:	Pop
Description:	Pop!_OS 22.04 LTS
Release:	22.04
Codename:	jammy

## Grupo 6
  O projeto deve ser feito por, no máximo, 3 pessoas. Estamos seguindo a numeração de grupos que está disponível em planilha desde o trabalho 1, de camada de enlace.
  O mesmo grupo será mantido para os 3 módulos do projeto.

| **NUSP** | **Membro do Grupo 6**    |
|----------|--------------------------|
| 12543544 | Danielle Modesti         |
| 10310227 | Hélio Nogueira Cardoso   |
| 10691331 | Théo da Mota dos Santos  |

## Introdução
  O protocolo IRC era amplamente utilizado na década de 90 e o é até hoje por alguns grupos de computação. Ele pode ser usado para criar uma aplicação de chat.
  O IRC tem sido desenvolvido em sistemas utilizando protocolo TCP/IP e é um sistema que **deve** suportar múltiplos clientes conectados em um único servidor, realizando multiplexação dos dados recebidos por eles.

## Objetivo
  Criar uma aplicação de chat IRC, implementando as diversas partes que compõem cliente e servidor IRC (*Internet Relay Chat*). 
  A implementação a ser feita adapta as especificações dadas pelo [RFC 1459](https://datatracker.ietf.org/doc/html/rfc1459), que define o IRC.

  Há diversos módulos que devem ser implementados para completude do trabalho. O código deve ser feito em C ou C++ padrão, compilando com gcc ou g++, respectivamente. Não é permitido o uso de bibliotecas externas.

## Avaliação
  Primeiro módulo - 2 pontos;
  
  Segundo módulo - 4 pontos;
  
  Terceiro módulo - 3 pontos.

  Podem ser dados até 2 pontos extras para bônus feitos no trabalho, de modo que ele valerá, no máximo, 11 pontos.

  
## O que deve ser entregue
- um arquivo .zip com todo o sistema com os códigos, um arquivo Makefile para compilar e executar o código e um README com os nomes dos membros do grupo, a versão do sistema operacional (Linux) e do compilador utilizado, além de quaisquer instruções adicionais que o grupo julgar necessárias.
- OUTRA OPÇÃO é entregar um repositório do GitHub com todos os arquivos acima listados.
  
![What should be delivered](/especification/what_to_deliver.png)

- Deve ser disponibilizado também um vídeo (link para o vídeo no youtube ou drive presente no README) e link deve estar aberto/acessível. No vídeo, todos os participantes devem explicar parte do ***que*** foi implementado e ***como*** foi implementado; o chat deve ser executado de modo que sejam apresentadas todas as suas funcionalidades.

# Módulo 1 - Implementação de *Sockets*
## Especificação da implementação
  Neste módulo será desenvolvida uma aplicação para a comunicação entre clientes. Para isso, deve ser implementado um *socket*, o qual define um mecanismo de troca de dados entre dois ou mais processos distintos, podendo eles estarem em execução na mesma máquina ou em máquinas diferentes, porém ligadas através da rede. Uma vez que a ligação entre dois processos esteja estabelecida, eles poderão enviar e receber mensagens um do outro.

  Na implementação a ser entregue devem ser implementados *sockets* TCP que permitam a comunicação entre duas aplicações, de modo que o usuário da segunda aplicação possa ler e enviar mensagens para o usuário da primeira aplicação, e vice-versa.
  Cada mensagem é limitada por um tamanho de 4096 caracteres. Assim, caso um usuário envie uma mensagem maior do que isso, ela deverá, automaticamente, ser dividida em múltiplas mensagens.

## Discussões e Resultados
  Foi possível implementar dois códigos, representando cada uma das duas aplicações que se comunicam para o módulo 1. Uma das aplicações foi chamada de 'client.cpp' e, a outra, 'server.cpp'. Primeiro, deve-se compilar e executar o código do server e, posteriormente, o do client. Após fazer isso, tendo as duas aplicações conectadas em uma mesma rede (modifique o IP, o segundo parâmetro da função inet_pton do client, para isso). Com tudo configurado, o cliente se conecta ao servidor e pode enviar mensagens a ele, alternando com mensagens enviadas pelo código de 'server', até que eles se desconectem, com '/exit'.

  O bom funcionamento do módulo 1 foi testado ao conectar, primeiro, duas abas de terminal, cada uma rodando um dos códigos, em uma mesma rede privada. Depois disso, dois membros do grupo foram capazes de se comunicar, cada um com seu computador pessoal, dentro de uma mesma rede privada; de mesmo modo, um executou o 'client' e o outro executou o 'server'. 

  A partir dos resultados obtidos, assume-se que é possível seguir para o módulo 2.

# Módulo 2 - Comunicação entre Múltiplos Clientes e Servidor
## Especificação da implementação
  Partindo da base criada no primeiro módulo, deve-se implementar um modelo de clientes-servidor que corresponda a um *chat*, de modo que uma mensagem de um cliente deve ser enviada para todos os clientes, passando por uma aplicação servidora.
  Cada cliente deve ter um apelido definido arbitrariamente, o qual, para este módulo, pode ser simplesmente um inteiro (*index*) ou uma *string* qualquer. As mensagens aparecerão para todos os usuários (inclusive para o seu emissor) no formato **apelido: mensagem**. Cada mensagem será separada por um '\n'. Cada mensagem ainda deve ser limitada por 4096 caracteres.
  Para fechar a conexão, um cliente pode enviar um comando de saída (/quit) ou um sinal de EOF (Ctrl + D).
  
### Comandos a serem implementados do cliente para o servidor
- /connect - Estabelece conexão com o servidor;

- /quit - o cliente fecha a conexão e fecha a aplicação;

- /ping - o servidor retorna "pong" assim que receber a mensagem.

### Pontos importantes
  O servidor deve checar se os clientes receberam as mensagem. Caso uma mensagem não seja recebida, ela deve ser enviada novamente. Após 5 falhas, o servidor deve fechar a conexão com o cliente.
  
  Trate *deadlocks* e possíveis problemas que possam surgir com o uso de *threads*. 

  Deve ser necessário lidar com SIGINT (Ctrl + C) no *chat*. Para isso, a sugestão é adicionar um *handler* que ignore o sinal ou imprima alguma mensagem.

# Módulo 3 - Implementação de Múltiplos Canais
## Especificação da implementação
  Finalmente, devem ser implementados múltiplos canais, além da função de administradores de canais.
  Ao abrir a aplicação, o usuário deverá, por meio do comando *join*, especificar em qual canal ele deseja se conectar. Caso o canal especificado não exista, ele deve ser criado, e o primeiro usuário a se conectar a ele se torna seu administrador. 

  O nome de um canal deve seguir restrições apresentadas no RFC-1459.

  Um administrador do canal tem permissão de usar comandos *kick*, *mute* e *whois* em usuários.

  Ao abrir a aplicação pela primeira vez, um usuário deve definir um apelido por meio do comando *nickname*, limitando o nome de usuário a 50 caracteres ASCII.

#### Comandos a serem implementados
  Além dos comandos dos módulos anteriores, devem ser implementados os seguintes:

  - /join nomeCanal - entra no canal de nome especificado;

  - /nickname apelidoDesejado - cliente passa a ser reconhecido pelo apelido especificado;

  - /ping - servidor retorna "pong" assim que receber a mensagem.
    
  #### Comandos executados apenas por administradores de canais (*chops*)
  - /kick nomeUsuario - fecha a conexão do usuário especificado;

  - /mute nomeUsuario - faz com que o usuário especificado não possa enviar mensagens neste canal;

  - /unmute nomeUsuario - volta a permitir que um usuário 'mudo' envie mensagens no canal;

  - /whois nomeUsuario - retorna o endereço IP do usuário **apenas para o administrador**.

# Item Bônus
  Canais podem ser restritos apenas para usuários convidados. O RFC indica esse funcionamento.

  O grupo pode implementar esta funcionalidade para um bônus na pontuação final do trabalho :-)
  
