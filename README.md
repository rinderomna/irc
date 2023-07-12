# Trabalho Camadas de Transporte e Aplicação - protocolo IRC

Repositório para o Trabalho 2 da disciplina "SSC0142 - Redes de Computadores" do ICMC - USP - São Carlos, ministrada pela professora Kalinka Regina Lucas Jaquie Castelo Branco.

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
