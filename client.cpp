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
char* username = "client1";

int main(int argc, char *argv[]) {
    /*
      if (strcmp(argv[1], "command\n")) {
        printf("Testando inputs\n");
        printf("Enter a command: ");
        scanf("%s", input);

      }
    */

    if (argc < 2) {
        fprintf(stderr, "usage %s hostname\n", argv[0]);
        exit(0);
    }

    int sockfd = createConnection(argv);
    char buffer[MAX_MESSAGE_LENGTH];

    MESSAGE a;
    strncpy(a.client, "client1", MAX_MESSAGE_LENGTH);
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
            running = 0;
            break;
        }

        bzero(buffer, 256);

        /* read from the socket */
        do {
            n = read(sockfd, buffer, MAX_MESSAGE_LENGTH);
        } while (n < MAX_MESSAGE_LENGTH);

        if (n < 0)
            printf("ERROR reading from socket\n");

        printf("Answer: %s\n", buffer);

    } while (running);
    printf("Ending connection\n");

    close(sockfd);
    return 0;
}

int createConnection(char *argv[]) {
    int sockfd, n;
    struct sockaddr_in serv_addr{};
    struct hostent *server;

    printf("%s", argv[1]);
    server = gethostbyname(argv[1]);
    if (server == nullptr) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
        printf("ERROR opening socket\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr = *((struct in_addr *) server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        printf("ERROR connecting\n");
    return sockfd;

}