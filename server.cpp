#include <pthread.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>

#include "./serverProcessor.cpp"

#define PORT 4000
#define MAX_CONNECTIONS_PER_CLIENT 2
#define MAX_TOTAL_CONNECTIONS 5

void *client_thread(void *arg);

int create_connection();

bool checkClientAcceptance(int sockfd);

void removeClientConnectionsCount(int sockfd);

void createSyncDir(const string& clientName);

map<string, int> clientsActiveConnections{};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
    struct sockaddr_in cli_addr{};
    int sockfd, newsockfd;
    socklen_t clilen;

    clilen = sizeof(struct sockaddr_in);
    sockfd = create_connection();

    // tells the socket that new connections shall be accepted
    listen(sockfd, MAX_TOTAL_CONNECTIONS);
    printf("Waiting accept\n");

    // get a new socket with a new incoming connection

    while (true) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd == -1)
            fprintf(stderr, "ERROR on accept\n");
        else {
            printf("Connection established successfully.\n\n");
            pthread_t th1;
            if (!checkClientAcceptance(newsockfd)) continue;

            pthread_create(&th1, nullptr, client_thread, &newsockfd);
        }
    }

    return 0;
}

void *client_thread(void *arg) {
    MESSAGE message;
    int sockfd = *(int *) arg;
    int running = 1;

    while (running) {
        if (!receiveAll(sockfd, (void *) &message, sizeof(MESSAGE))) {
            fprintf(stderr, "ERROR reading from socket\n");
            return (void *) -1;
        }

        if (!strcmp(message.content, "exit")) {
            printf("Ending Connection\n");
            running = 0;
        } else {
            ServerProcessor handler = *new ServerProcessor(sockfd, message);
            handler.handleInput();
        }
    }

    removeClientConnectionsCount(sockfd);

    close(sockfd);

    printf("Connection ended\n");

    return nullptr;
}

int create_connection() {
    int sockfd, bindReturn;
    struct sockaddr_in serv_addr{};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "ERROR opening socket\n");
        exit(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr.sin_zero), 8);

    bindReturn = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (bindReturn < 0) {
        fprintf(stderr, "ERROR on binding\n");
        exit(-1);
    }
    return sockfd;

}

bool checkClientAcceptance(int sockfd) {
    bool accepted = true;
    MESSAGE message;

    if (!receiveAll(sockfd, &message, sizeof(MESSAGE))) {
        fprintf(stderr, "ERROR reading from socket\n");
    }

    string clientName = message.client;
    strcpy(message.content, "accepted\0");

    pthread_mutex_lock(&mutex);

    size_t clientConnectionsAmount = clientsActiveConnections.count(clientName);

    if (clientConnectionsAmount) {
        if (clientsActiveConnections[clientName] == MAX_CONNECTIONS_PER_CLIENT) {
            fprintf(stderr, "WARNING: %s exceeded connections quota\n", clientName.c_str());
            strcpy(message.content, "denied\0");
            accepted = false;
        } else {
            clientsActiveConnections[clientName]++;
        }
    } else {
        clientsActiveConnections.insert(make_pair(clientName, 1));
    }

    pthread_mutex_unlock(&mutex);

    if (!sendAll(sockfd, &message, sizeof(MESSAGE))) {
        fprintf(stderr, "ERROR writing to socket\n");
    }

    if (accepted) {
        createSyncDir(clientName);
    }

    return accepted;
}

void removeClientConnectionsCount(int sockfd) {
    MESSAGE message;

    if (!receiveAll(sockfd, &message, sizeof(MESSAGE))) {
        fprintf(stderr, "ERROR reading from socket\n");
        return;
    }

    pthread_mutex_lock(&mutex);
    string clientName = message.client;
    clientsActiveConnections[clientName]--;
    pthread_mutex_unlock(&mutex);
}

void createSyncDir(const string& clientName) {
    string dirPath = "sync_dir_" + clientName;

    mkdir(dirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}
