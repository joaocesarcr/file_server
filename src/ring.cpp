#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "../include/message_struct.hpp"
#include "../include/utils.hpp"

using namespace std;

int listen_to_server(void* arg);
void heart_beat_s(void* arg);
void election(void* arg);
void listen_to_ring(void* arg);
void resetConnection(int sockfd);
void connect_to(struct sockaddr_in ip);
bool isInParticipants(int pid);

/* Pra cada anel:
heart_beat server
listen_to_server
escuta outro anel
propaga pro outro anel
*/

bool SERVER_DIED = false;
bool ELECTION_RUNNING = false;
bool RESET_CONNECTION = false;

vector<int> RING_PARTICIPANTS;
int RING_PARTICIPANTS_QTT = 0;

int MYPID = 0;
void ring() {
    socklen_t clilen = sizeof(struct sockaddr_in);
    struct sockaddr_in cli_addr{};
    int sockfd;
    listen(sockfd, MAX_TOTAL_CONNECTIONS);
    int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd == -1) {
        printf("ERROR on accept\n");
        return;
    }
    MYPID = getpid();
    pthread_t th1, th2, th3, th4;
    pthread_create(&th1, nullptr, reinterpret_cast<void *(*)(void *)>(heart_beat_s), &newsockfd);
    pthread_create(&th2, nullptr, reinterpret_cast<void *(*)(void *)>(listen_to_server), &newsockfd);
    pthread_create(&th3, nullptr, reinterpret_cast<void *(*)(void *)>(listen_to_ring), &newsockfd);
    pthread_create(&th4, nullptr, reinterpret_cast<void *(*)(void *)>(election), &newsockfd);
}

int listen_to_server(void* arg) {
//    new_thread heart_beat;
    SERVER_MSG message;
    int sockfd = *(int *) arg;
    int running = 1;

    while (running) {
        if (!receiveAll(sockfd, (void *) &message, sizeof(MESSAGE))) {
            return 1;
        }

        // Switchcase ou usar strings
        if (message.command == 2) {
            printf("Ending Connection\n");
            running = 0;

        } else if (message.command == 0) {
            resetConnection(sockfd); // mata a thread de escuta outro anel

        } else if (message.command == 1) {
            connect_to(message.add);
        }
    }

    close(sockfd);
    printf("Connection ended\n");

    // Avisa para o server que desconectei
    //
    return 0;
}

void resetConnection(int sockfd) {
    while(1) {
        if (RESET_CONNECTION) {
            RESET_CONNECTION = false;
            close(sockfd);

            // Abrir nova conexao
//            connect_to( ??? );
        }
    }
}

void connect_to(struct sockaddr_in ip) {

}

void heart_beat_s(void* arg) {
    MESSAGE message;
    int host = *(int *) arg;
    strcpy(message.content, "alive?\0");
    bool running = true;

    // precisa de resposta do host?
    do {
        sleep(10);
        if (!sendAll(host, &message, sizeof(MESSAGE)))
            running = false;
    } while (running);
    SERVER_DIED = true; // compartilhada
}

void election(void* arg) {
    while(1) {
        if (SERVER_DIED && !ELECTION_RUNNING) {
            ELECTION_RUNNING = true;
            beginElection();
        }
    }
}

void listen_to_ring(void* arg) {
    int sockfd = *(int *) arg;
    RING_MSG message;
    if (!receiveAll(sockfd, (void *) &message, sizeof(RING_MSG))) {

    }
    ELECTION_RUNNING = true;

    if (message.command == 1) {
        // Mensagem de anel eleito
        sendAll(sockfd, &message, sizeof(MESSAGE));
        ELECTION_RUNNING = false;
    }

    if (message.pid == mypid) {
        // Fui eleito
    }

    if (!isInParticipants(message.pid) && (message.command == 1)) {
        RING_PARTICIPANTS[RING_PARTICIPANTS_QTT] = pid;
        RING_PARTICIPANTS_QTT++;
        if (message.pid > MYPID) {

        }
    }
}

bool isInParticipants(int pid) {
    for (int i : RING_PARTICIPANTS) {
        if (i == pid) {
            return true;
        }
    }
    return false;
}