#include <vector>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <string.h>

typedef struct server_msg_s {
    struct sockaddr_in add{};
    int command;
    int port;
} SERVER_MSG;

typedef struct heart_beat_Struct {
    int ignore; // dps voltar pra string
} HEART_BEAT;

using namespace std;
vector<int> users_sockets;
vector<struct sockaddr_in> user_ips;
vector<int> user_ports;

int create_connection(int port);
int users_qtt = 0;
int PORT = 4050;
int new_ring_socket, new_hb_socket;
struct sockaddr_in cli_addr{};
socklen_t clilen;

void* talkto();
void* heart_beat_s(void* arg);
bool receiveAll(int socket, void *buffer, size_t length);
bool sendAll(int socket, const void *buffer, size_t length);
int host();

int main() {
    host();
    return 0;
}
int host() {
    printf("Criando conexao\n");
    int sockfd = create_connection(PORT+5);
    int socket_heartbeat = create_connection(PORT+6);
    printf("Iniciando listen\n");
    listen(sockfd, 20);
    listen(socket_heartbeat, 20);

    while (true) {
        new_ring_socket = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        new_hb_socket = accept(socket_heartbeat, (struct sockaddr *) &cli_addr, &clilen);

        if (new_ring_socket == -1) {
            printf("ERROR on accept\n");
            continue;
        }
        printf("Active rings: %d\n",users_qtt + 1);

        users_sockets.push_back(new_ring_socket);
        user_ips.push_back(cli_addr);
        user_ports.push_back(PORT + users_qtt + 1);

        SERVER_MSG to_connect_to_first;
        to_connect_to_first.command = 1;
        to_connect_to_first.add = user_ips[0];
        to_connect_to_first.port = user_ports[0];

        SERVER_MSG to_connect_to_new;
        to_connect_to_new.command = 1;
        to_connect_to_new.port = PORT + users_qtt + 1;
        to_connect_to_new.add = cli_addr;

        SERVER_MSG useport;
        useport.port = PORT + users_qtt + 1;
        sendAll(new_ring_socket, &useport, sizeof(SERVER_MSG));

        // Caso especial: se só tem uma conexao?
        if (users_qtt == 0) {
            //printf("Vc eh o primeiro usuario do anel\n");
            // Use a porta

        } else if (users_qtt == 1) {
            //printf("Vc eh o segundo usuario do anel\n");

            /*
            printf("users_sockets[0] = %d\n", users_sockets[0]);
            printf("user_ports[0] = %d\n", user_ports[0]);
            printf("users_sockets[%d] = %d\n", users_qtt, users_sockets[users_qtt]);
            printf("user_ports[0] = %d\n", user_ports[users_qtt]);
            */

//          sendAll(users_sockets[0], &to_accept_new, sizeof(SERVER_MSG));
//          sendAll(users_sockets[users_qtt], &to_accept_new, sizeof(SERVER_MSG));

            sendAll(users_sockets[0], &to_connect_to_new, sizeof(SERVER_MSG));
            sendAll(users_sockets[users_qtt], &to_connect_to_first, sizeof(SERVER_MSG));

        } else if (users_qtt > 1) {
            // tell users_sockets[0] to_accept_new
            // sendAll(users_sockets[0], &to_accept_new, sizeof(SERVER_MSG));

            // tell users_sockets[-1] to connect_to newuser
            sendAll(users_sockets[users_qtt - 1], &to_connect_to_new, sizeof(SERVER_MSG));

            // tell newuser to_accept_new
            // sendAll(users_sockets[users_qtt], &to_accept_new, sizeof(SERVER_MSG));
            // tell newuser to connect to users_sockets[0]
            sendAll(new_ring_socket, &to_connect_to_first, sizeof(SERVER_MSG));
        }

        // Cria thread pra heart_beat e comandos
        // printf("Connection established successfully.\n\n");
        pthread_t th1,th2;
        pthread_create(&th1, nullptr, reinterpret_cast<void *(*)(void *)>(heart_beat_s), &new_hb_socket);
        //        pthread_create(&th1, nullptr, reinterpret_cast<void *(*)(void *)>(talkto), &newsockfd2);
        users_qtt++;
    }
    return 0;
}
void* talkto() {
    // A ser utilizado para lidar com desconexoes?

}

void* heart_beat_s(void *arg) {
    HEART_BEAT message;
    int sockfd = *(int *) arg;
    bool running = true;
    while (running) {
        if (receiveAll(sockfd, &message, sizeof(HEART_BEAT)) <= 0)
            running = false;
        else {
            printf("HB \n");
        }
    }
}

int create_connection(int port) {
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

