#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sstream>
#include <vector>


#include "./h/message_struct.hpp"

#define PORT 4000

using namespace std;

int createConnection(char *argv[]);

bool checkConnectionAcceptance(int socket);

std::vector<std::string> splitString(const std::string &str);
// ./myClient <username> <server_ip_address> <port>

int main(int argc, char *argv[]) {
    /*
      if (strcmp(argv[1], "content\n")) {
        printf("Testando inputs\n");
        printf("Enter a content: ");
        scanf("%s", input);

      }
    */
    if (argc < 4) {
        fprintf(stderr, "usage %s hostname\n", argv[0]);
        exit(-1);
    }

    int sockfd = createConnection(argv);
    char buffer[MAX_MESSAGE_LENGTH];

    MESSAGE message;
    strncpy(message.client, argv[1], MAX_MESSAGE_LENGTH);
    int running = 1;
    do {
        ssize_t n;
        std::string temp;
        printf("%s: ", message.client);
        getline(std::cin, temp);
        strcpy(message.content, temp.c_str());

        if (!strcmp(message.content, "exit")) {
            printf("Ending Connection\n");
            running = 0;
        }

        n = write(sockfd, (void *) &message, sizeof(MESSAGE));
        if (n < 0)
            printf("ERROR writing to socket\n");


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

    if (!checkConnectionAcceptance(sockfd)) exit(-1);

    printf("Connection established successfully.\n\n");
    return sockfd;

}

std::vector<std::string> splitString(const std::string &str) {
    string s;

    stringstream ss(str);

    vector<string> v;
    while (getline(ss, s, ' ')) {
        if (s != " ") {
            v.push_back(s);
        }
    }

    return v;
}

bool checkConnectionAcceptance(int socket) {
    size_t n;
    MESSAGE message;
    do {
        n = read(socket, (void *) &message, sizeof(message));
    } while (n < sizeof(message));

    if (strcmp(message.content, "accepted\0") == 0) return true;

    printf("ERROR: Connections quota reached\n");

    return false;
}

