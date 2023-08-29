#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include <string.h>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>

int PORT =  4050;
bool BEGIN_ELECTION = false;
int SEND_TO_SOCKET = 0;
int LISTENING_SOCKET = 0;
bool GOT_PORT = false;
bool SERVER_DIED = false;
bool ELECTION_RUNNING = false;
int MYPID = 0;
std::vector<int> RING_PARTICIPANTS;
int RING_PARTICIPANTS_QTT = 0;

using namespace std;
vector<struct sockaddr_in> user_ips;
vector<int> user_ports;

pthread_mutex_t serverDiedMutex = PTHREAD_MUTEX_INITIALIZER; // Initialize mutex


struct ThreadArgs {
    int socket;
    std::string message;
};

typedef struct server_msg_s {
    struct sockaddr_in add;
    int command;
    int port;
} SERVER_MSG;

typedef struct heart_beat_S {
    bool ignore; // dps voltar pra string
} HEART_BEAT;

typedef struct ring_msg_S {
    int command; // 0= send PID; 1= ELECTED
    int pid;
} RING_MSG;

void* neverdie(void* arg);
bool receiveAll(int socket, void *buffer, size_t length);
void* listen_to_server(void* arg);
bool sendAll(int socket, const void *buffer, size_t length);
void* heart_beat(void* arg) ;
void handleServerCommand(SERVER_MSG message);
int createConnectionRing(char argv[], int port) ;
int host_connectionRing(int port);
void* ring_commands(void* arg);
void* receive_connections_thread(void* arg);
int connec_to_ring(struct sockaddr_in, int port);
bool isInParticipants(int pid);

int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    MYPID = getpid();
    printf("MY PID = %d\n",MYPID);

    if (argc < 2) {
        fprintf(stderr,"usage %s hostname\n", argv[0]);
        exit(0);
    }

    int listen_commands = createConnectionRing(argv[1], PORT + 5);
    int heart_beat_connection = createConnectionRing(argv[1], PORT + 6);

    pthread_t threadListener, threadHB, acceptConnections, neverdie_t;
    auto *listenerArgs = new ThreadArgs{listen_commands, argv[1]};
    auto *hbArgs = new ThreadArgs{heart_beat_connection, argv[1]};

    // Mutex
    pthread_create(&threadListener, nullptr, listen_to_server, listenerArgs);
    pthread_create(&neverdie_t, nullptr, neverdie, listenerArgs);
    pthread_create(&acceptConnections, nullptr, receive_connections_thread,NULL);
//  pthread_create(&threadHB, nullptr, heart_beat, hbArgs);


//  pthread_join(threadHB, nullptr);
    pthread_join(acceptConnections, nullptr);
    pthread_join(threadListener, nullptr);
    pthread_join(neverdie_t, nullptr);
    /*
    */
    printf("CHEGOU NO FINAL DO PROGRAMA \n");

}

void* neverdie(void* arg) {
    while(true) {
        sleep(3);
//    printf("Never die\n");
    }
}
void* listen_to_server(void* arg) {
    SERVER_MSG message;
    int sockfd = *(int *) arg;
    bool running = true;
    int n = 0;
    RING_MSG begin;

    n = receiveAll(sockfd, &message, sizeof(SERVER_MSG));

    PORT = message.port;
    GOT_PORT = true;
    while (running) {
        pthread_mutex_lock(&serverDiedMutex); // Acquire the mutex
        if (SERVER_DIED) {
            pthread_mutex_unlock(&serverDiedMutex); // Release the mutex before exiting
            running = false;
        }
        pthread_mutex_unlock(&serverDiedMutex); // Release the mutex

        n = receiveAll(sockfd, &message, sizeof(SERVER_MSG));
        if (n <= 0) {
            if (!ELECTION_RUNNING) {
                // Começa nova eleicao caso nao exista uma
                printf("SERVER DIED LISTENING FOR COMMAND!\n");
                SERVER_DIED = true;
                RING_PARTICIPANTS.push_back(MYPID);
                RING_PARTICIPANTS_QTT++;
                printf("STARTED ELECTION!\n");
                begin.command = 0;
                begin.pid=MYPID;
                sendAll(SEND_TO_SOCKET, &begin, sizeof(RING_MSG));
                ELECTION_RUNNING = true;
            }

            close(sockfd);
            pthread_exit(nullptr);

        } else {
            handleServerCommand(message);
        }
    }

    close(sockfd);
}

void handleServerCommand(SERVER_MSG message) {
    char* ipAddress = inet_ntoa(message.add.sin_addr);
//  printf("Comando: %d\nIp: %s\nPort: %d\n", message.command, ipAddress,message.port);

    // Desfaça receba
    if (message.command == 0) {


    }

        // Conect to
        // MUTEX AQUI
    else if (message.command == 1) {
        if (SEND_TO_SOCKET != 0) {
            close(SEND_TO_SOCKET);
        }
        printf("Connecting to port = %d\n",message.port);
        int sockfd = connec_to_ring(message.add, message.port);
        SEND_TO_SOCKET = sockfd;
        // Handle input de envio aqui!
    }

        // Die
    else if (message.command == 2) {
        // Mutex pra terminar conexao

    }

    else if (message.command == 3) {
        user_ips.push_back(message.add);
        user_ports.push_back(message.port);


    }
}

int connec_to_ring(struct sockaddr_in name, int port) {
    int sockfd;

    struct sockaddr_in serv_addr;
    char* ipAddress = inet_ntoa(name.sin_addr);
    struct hostent *server;
    server = gethostbyname(ipAddress);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host, exiting\n");
        exit(0);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        printf("ERROR opening socket\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = *((struct in_addr *) server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);


    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR establishing connection. Exiting...\n");
        exit(-1);
    }
    printf("Connected to send info to another ring. Socket = %d\n", sockfd);

    return sockfd;


}

void* receive_connections_thread(void* arg) {
    // ENQUANTO NAO SEI PORTA ESPERA
    while (!GOT_PORT) {
    }
    printf("RECEBI PORTA DO HOST = %d\n",PORT);

    bool running = true;
    socklen_t clilen = sizeof(struct sockaddr_in);
    struct sockaddr_in cli_addr{};

    int socket_listen = host_connectionRing(PORT);
    listen(socket_listen, 5);


    pthread_t th1;
    printf("Waiting connection\n");

    while (running) {
        int socket = accept(socket_listen, (struct sockaddr *) &cli_addr, &clilen);
        if (LISTENING_SOCKET) {
            close(LISTENING_SOCKET);
        }
        LISTENING_SOCKET = socket;

        unsigned short client_port = ntohs(cli_addr.sin_port);
        char client_ip[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &(cli_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        printf("Client connected from IP %s and port %d. Socket = %d\n", client_ip, client_port, LISTENING_SOCKET);
        pthread_create(&th1, nullptr, ring_commands, &LISTENING_SOCKET);
    }
}

void* ring_commands(void* arg) {
    RING_MSG message;
    int sockfd = *(int *) arg;
    bool running = true;
    int pid;
    while (running) {
        printf("Waiting for commands from ring\n");
        if (receiveAll(sockfd, &message, sizeof(RING_MSG)) <= 0) {
            running = false;
            printf("ERROR READING FROM ANOTHER RING\n");
        }

        else {
            printf("Mensagem recebida: \n");
            printf("Comando: %d\nPid: %d\n", message.command,message.pid);
            ELECTION_RUNNING = true;


            // 1: confirma eleito
            if (message.command == 1) {
                // Confirma eleicao
                if (message.pid == MYPID) {
                    printf("FUI ELEITO!!\n");
                    // Avisa para ele se conectar a minha conexao
                    // ATIVA MODO HOST
                }
                else {
                    // Repassa o que foi recebido
                    printf("Repassando que %d foi eleito\n",message.pid);
                    sendAll(SEND_TO_SOCKET, &message, sizeof(RING_MSG));
                    // Se conecta ao novo host
                    // Reseta participantes e eleicao
                }
                ELECTION_RUNNING = false;
            }
                // 0: eleicao: mensagem de PID
            else {
                if (message.pid == MYPID) {
                    // Mensagem de anel eleito
                    printf("Avisando que fui eleito\n");
                    message.command = 1;
                    sendAll(SEND_TO_SOCKET, &message, sizeof(RING_MSG));

                }
                    // Se eu já mandei e meu pid é maior, nao envia denovo
                    // Se o dele é maior e nao enviei, envia o dele
                else if (!isInParticipants(message.pid)) {
                    printf("Mensagem de um PID nao estava nos participantes: \n");
                    RING_PARTICIPANTS.push_back(message.pid);
                    RING_PARTICIPANTS_QTT++;
                    if (message.pid > MYPID) {
                        sendAll(SEND_TO_SOCKET, &message, sizeof(RING_MSG));
                        printf("Mensagem enviada:\n pid: %d\n comando: %d\n",message.pid,message.command);
                    }
                } else {
                    printf("%d Tava nos participantes\n", pid);
                    for(int i =0; i <= RING_PARTICIPANTS_QTT;i++) {
                        printf("Participant %d = %d\n",i, RING_PARTICIPANTS[i]);
                    }
                }

            }
        }
    }
}


void* heart_beat(void* arg) {
    HEART_BEAT message;
    int host = *(int *) arg;
    bool running = true;

    do {
        sleep(1);
        if (!sendAll(host, &message, sizeof(HEART_BEAT))) {
            printf("SERVER DIED SENDING HEART_BEAT!\n");
            close(host);
            if (!ELECTION_RUNNING)
                // BEGIN_ELECTION = true;
                printf("SERVER DIED END!\n");
            pthread_exit(nullptr);
            //running = false;
            //SERVER_DIED = true;
        }
    } while (running);
}

bool receiveAll(int socket, void *buffer, size_t length) {
    char *data = static_cast<char *>(buffer);
    ssize_t totalReceived = 0;

    while (totalReceived < length) {
        ssize_t received = read(socket, data + totalReceived, length - totalReceived);

        if (received < 1) return false;

        totalReceived += received;
    }

    return true;
}

bool sendAll(int socket, const void *buffer, size_t length) {
    const char *data = static_cast<const char *>(buffer);
    ssize_t totalSent = 0;

    while (totalSent < length) {
        ssize_t sent = write(socket, data + totalSent, length - totalSent);

        if (sent == -1) return false;

        totalSent += sent;
    }

    return true;
}


int createConnectionRing(char name[], int port) {
    int sockfd;
    struct sockaddr_in serv_addr{};
    struct hostent *server;

    server = gethostbyname(name);
    if (!server) {
        fprintf(stderr, "ERROR, no such host, exiting\n");
        exit(-1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        fprintf(stderr, "ERROR opening socket, exiting\n");
        exit(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = *((struct in_addr *) server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR establishing connection. Exiting...\n");
        exit(-1);
    }

    return sockfd;
}

int host_connectionRing(int port) {
    int sockfd;
    struct sockaddr_in serv_addr{};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "ERROR opening socket\n");
        exit(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr.sin_zero), 8);
    int bindReturn = ::bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (bindReturn < 0) {
        fprintf(stderr, "ERROR on binding\n");
        exit(-1);
    }
    return sockfd;
}
bool isInParticipants(int pid) {
    for (int i =0; i <= RING_PARTICIPANTS_QTT;i++) {
        if (RING_PARTICIPANTS[i] == pid) {
            return true;
        }
    }
    return false;
}
/*
*/