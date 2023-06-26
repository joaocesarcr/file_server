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

#define PORT 4000

int createConnection(char *argv[]);

// ./myClient <username> <server_ip_address> <port>

int main(int argc, char *argv[]) {
    /*
      if (strcmp(argv[1], "command\n")) {
        printf("Testando inputs\n");
        printf("Enter a command: ");
        scanf("%s", input);

      }
    */
    if (argc < 4) {
        fprintf(stderr, "usage %s hostname\n", argv[0]);
        exit(-1);
    }

    int sockfd = createConnection(argv);
    char buffer[MAX_MESSAGE_LENGTH];

    MESSAGE a;
    strncpy(a.client, argv[1], MAX_MESSAGE_LENGTH);
    int running = 1;
    do {
        ssize_t n;
        //strncpy(a.command, "Sending packet", MAX_MESSAGE_LENGTH);
        //printf("teste: %s\n", a.data);
        std::string temp;
        getline(std::cin, temp);
        strcpy(a.command, temp.c_str());

        /* write in the socket */
        n = write(sockfd, (void *) &a, sizeof(MESSAGE));
        if (n < 0)
            printf("ERROR writing to socket\n");
        if (!strcmp(a.command, "exit")) {
            close(sockfd);
            break;
        }

        bzero(buffer, 256);

        /* read from the socket */
        do {
            n = read(sockfd, buffer, MAX_MESSAGE_LENGTH);
        } while (n < MAX_MESSAGE_LENGTH);

        printf("Answer: %s\n\n", buffer);

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

    printf("Connection established successfully.\n\n");
    return sockfd;

}
