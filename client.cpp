#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "./h/message_struct.hpp"
#include "./clientProcessor.cpp"

#define PORT 4000

using namespace std;

int createConnection(char *argv[]);

bool checkConnectionAcceptance(char clientName[], int socket);

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "usage %s hostname\n", argv[0]);
        exit(-1);
    }

    int sockfd = createConnection(argv);

    MESSAGE message;
    strncpy(message.client, argv[1], MAX_MESSAGE_LENGTH);
    int running = 1;
    do {
        ssize_t n;
        string temp;
        printf("%s: ", message.client);
        getline(cin, temp);
        strcpy(message.content, temp.c_str());

        if (!strcmp(message.content, "exit")) {
            running = 0;
        }

        n = write(sockfd, (void *) &message, sizeof(MESSAGE));
        if (n < 0)
            printf("ERROR writing to socket\n");

        ClientProcessor handler = *new ClientProcessor(sockfd, message);
        handler.handleInput();


    } while (running);
    printf("Ending connection\n");

    close(sockfd);
    return 0;
}

int createConnection(char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr{};
    struct hostent *server;

    server = gethostbyname(argv[2]);
    if (!server) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(-1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        printf("ERROR opening socket\n");
        exit(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr = *((struct in_addr *) server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("ERROR establishing connection. Exiting...\n");
        exit(-1);
    }

    if (!checkConnectionAcceptance(argv[2], sockfd)) exit(-1);

    printf("Connection established successfully.\n\n");
    return sockfd;

}

bool checkConnectionAcceptance(char clientName[], int socket) {
    ssize_t n;
    MESSAGE message;
    strcpy(message.client, clientName);

    n = write(socket, (void *) &message, sizeof(MESSAGE));
    if (n < 0)
        printf("ERROR writing to socket\n");

    do {
        n = read(socket, (void *) &message, sizeof(MESSAGE));
    } while (n < sizeof(MESSAGE));

    if (strcmp(message.content, "accepted\0") == 0) return true;

    printf("ERROR: Connections quota reached\n");

    return false;
}

