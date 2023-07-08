#include <pthread.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>

#include "./handleClientInput.cpp"

#define PORT 4000
#define MAX_CONNECTIONS_PER_CLIENT 2
#define MAX_TOTAL_CONNECTIONS 5

typedef struct clientArgs {
    map<string, int> clients;
    int socket;
    size_t activeConnectionsAmount;
} clientArgs;

void *client_thread(void *arg);

int create_connection();

bool checkClientAcceptance(clientArgs *args);

int main(int argc, char *argv[]) {
    size_t activeConnectionsAmount = 0;
    struct sockaddr_in cli_addr{};
    map<string, int> clients{};
    int sockfd, newsockfd;
    socklen_t clilen;

    clilen = sizeof(struct sockaddr_in);
    sockfd = create_connection();

    // tells the socket that new connections shall be accepted
    listen(sockfd, 5);
    printf("Waiting accept\n");

    // get a new socket with a new incoming connection

    while (true) {
        if (activeConnectionsAmount == MAX_TOTAL_CONNECTIONS) {
            printf("Limits of active connections reached.\n");
            continue;
        }

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd == -1)
            printf("ERROR on accept\n");
        else {
            printf("Connection established successfully.\n\n");
            pthread_t th1;
            clientArgs args = {clients, newsockfd, activeConnectionsAmount};
            pthread_create(&th1, nullptr, client_thread, &args);
            activeConnectionsAmount++;
        }
    }

    return 0;
}

void *client_thread(void *arg) {
    MESSAGE message;
    auto *args = (clientArgs *) arg;
    int newsockfd = (*args).socket;
    int running = 1;
    ssize_t n;

    if (!checkClientAcceptance(args)) return nullptr;

    while (running) {
        do {
            n = read(newsockfd, (void *) &message, sizeof(message));
        } while (n < sizeof(message));

        if (n < 0) {
            printf("ERROR reading from socket\n");
            return (void *) -1;
        }
        if (!strcmp(message.content, "exit")) {
            printf("Ending Connection\n");
            running = 0;
        } else {
            ServerProcessor handler = *new ServerProcessor(newsockfd, message);
            handler.handleInput();
        }
    }

    close(newsockfd);
    (*args).activeConnectionsAmount--;

    printf("Connection ended\n");

    return nullptr;
}

int create_connection() {
    int sockfd, bindReturn;
    struct sockaddr_in serv_addr{};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("ERROR opening socket\n");
        exit(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr.sin_zero), 8);

    //  assigns the address specified by addr to the socket referred to
    // by the file descriptor sockfd
    bindReturn = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (bindReturn < 0) {
        printf("ERROR on binding\n");
        exit(-1);
    }
    return sockfd;

}

bool checkClientAcceptance(clientArgs *args) {
    bool accepted = true;
    MESSAGE message;
    ssize_t n;

    do {
        n = read(socket, (void *) &message, sizeof(message));
    } while (n < sizeof(message));

    string clientName = message.client;
    strcpy(message.content, "accepted\0");
    size_t clientConnectionsAmount = (*args).clients.count(clientName);

    if (clientConnectionsAmount) {
        if (clientConnectionsAmount == MAX_CONNECTIONS_PER_CLIENT) {
            printf("ERROR: %s exceeded connections quota\n", clientName.c_str());
            strcpy(message.content, "denied\0");
            accepted = false;
        } else {
            (*args).clients[clientName]++;
        }
    } else {
        (*args).clients.insert(make_pair(clientName, 1));
    }

    n = write(socket, (void *) &message, sizeof(MESSAGE));
    if (n < 0) {
        printf("ERROR writing to socket\n");
    }

    return accepted;
}
