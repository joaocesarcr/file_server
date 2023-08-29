#include "../include/ring.hpp"
#include "../include/server.hpp"

int PORT;
int SEND_TO_SOCKET = 0;
int LISTENING_SOCKET = 0;
bool GOT_PORT = false;
bool SERVER_DIED = false;
bool ELECTION_RUNNING = false;
int MYPID = 0;
vector<int> RING_PARTICIPANTS;
int RING_PARTICIPANTS_QTT = 0;

vector<struct sockaddr_in> user_ips_in_ring;
vector<int> user_ports_in_ring;

pthread_mutex_t serverDiedMutex = PTHREAD_MUTEX_INITIALIZER; // Initialize mutex

void ring(void *arg) {
    char **argv = (*(HostArgs *) arg).argv;
    PORT = (*(HostArgs *) arg).port;
    printf("%d\n", PORT);
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    MYPID = getpid();
    printf("MY PID = %d\n", MYPID);

    printf("waiting\n");
    printf("%s", argv[2]);
    int listen_commands = createConnectionRing(argv[2], PORT + 5);
    printf("done\n");

    pthread_t threadListener, threadHB, acceptConnections, neverdie_t;
    auto *listenerArgs = new ThreadArgs{listen_commands, argv[1]};

    // Mutex
    pthread_create(&threadListener, nullptr, reinterpret_cast<void *(*)(void *)>(listen_to_server), listenerArgs);
    pthread_create(&acceptConnections, nullptr, reinterpret_cast<void *(*)(void *)>(receive_connections_thread),
                   nullptr);

    pthread_join(acceptConnections, nullptr);
    pthread_join(threadListener, nullptr);

    printf("CHEGOU NO FINAL DO PROGRAMA \n");
}

void listen_to_server(void *arg) {
    SERVER_MSG message;
    ThreadArgs threadArgs = *(ThreadArgs *) arg;
    int sockfd = threadArgs.socket;
    bool running = true;
    int n;
    RING_MSG begin;

    printf("%i", sockfd);
    printf("waiting\n");
    receiveAll(sockfd, &message, sizeof(SERVER_MSG));
    printf("done ring\n");

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
                begin.pid = MYPID;
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
    char *ipAddress = inet_ntoa(message.add.sin_addr);
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
        printf("Connecting to port = %d\n", message.port);
        int sockfd = connec_to_ring(message.add, message.port);
        SEND_TO_SOCKET = sockfd;
        // Handle input de envio aqui!
    }

        // Die
    else if (message.command == 2) {
        // Mutex pra terminar conexao

    } else if (message.command == 3) {
        user_ips_in_ring.push_back(message.add);
        user_ports_in_ring.push_back(message.port);


    }
}

int connec_to_ring(struct sockaddr_in name, int port) {
    int sockfd;

    struct sockaddr_in serv_addr;
    char *ipAddress = inet_ntoa(name.sin_addr);
    struct hostent *server;
    server = gethostbyname(ipAddress);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host, exiting\n");
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

void receive_connections_thread(void *arg) {
    // ENQUANTO NAO SEI PORTA ESPERA
    while (!GOT_PORT) {
    }
    printf("RECEBI PORTA DO HOST = %d\n", PORT);

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
        pthread_create(&th1, nullptr, reinterpret_cast<void *(*)(void *)>(ring_commands), &LISTENING_SOCKET);
    }

}

void ring_commands(void *arg) {
    RING_MSG message;
    int sockfd = *(int *) arg;
    bool running = true;
    int pid;
    while (running) {
        printf("Waiting for commands from ring\n");
        if (receiveAll(sockfd, &message, sizeof(RING_MSG)) <= 0) {
            running = false;
            printf("ERROR READING FROM ANOTHER RING\n");
        } else {
            printf("Mensagem recebida: \n");
            printf("Comando: %d\nPid: %d\n", message.command, message.pid);
            ELECTION_RUNNING = true;


            // 1: confirma eleito
            if (message.command == 1) {
                // Confirma eleicao
                if (message.pid == MYPID) {
                    printf("FUI ELEITO!!\n");
                    // Avisa para ele se conectar a minha conexao
                    pthread_t th1;
                    auto *hostArgs = new HostArgs({PORT, nullptr});
                    pthread_create(&th1, nullptr, reinterpret_cast<void *(*)(void *)>(server), &hostArgs);
                } else {
                    // Repassa o que foi recebido
                    printf("Repassando que %d foi eleito\n", message.pid);
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
                        printf("Mensagem enviada:\n pid: %d\n comando: %d\n", message.pid, message.command);
                    }
                } else {
                    printf("%d Tava nos participantes\n", pid);
                    for (int i = 0; i <= RING_PARTICIPANTS_QTT; i++) {
                        printf("Participant %d = %d\n", i, RING_PARTICIPANTS[i]);
                    }
                }

            }
        }
    }
}

int createConnectionRing(char name[], int port) {
    int sockfd;
    struct sockaddr_in serv_addr{};
    struct hostent *server;

    printf("---%s---", name);
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
    for (int i = 0; i <= RING_PARTICIPANTS_QTT; i++) {
        if (RING_PARTICIPANTS[i] == pid) {
            return true;
        }
    }
    return false;
}
/*
*/