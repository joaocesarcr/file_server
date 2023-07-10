#include "./server.hpp"

map<string, int> clientsActiveConnections{};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
    struct sockaddr_in cli_addr{};
    int sockfd, newsockfd;
    int sockfd2, newsockfd2;
    int sockfd3, newsockfd3;
    socklen_t clilen;
    MESSAGE message;

    clilen = sizeof(struct sockaddr_in);
    sockfd = create_connection(PORT);
    sockfd2 = create_connection(PORT + 1);
    sockfd3 = create_connection(PORT + 2);

    // tells the socket that new connections shall be accepted
    listen(sockfd, MAX_TOTAL_CONNECTIONS);
    listen(sockfd2, MAX_TOTAL_CONNECTIONS);
    listen(sockfd3, MAX_TOTAL_CONNECTIONS);
    printf("Waiting accept\n");

    // get a new socket with a new incoming connection

    while (true) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        message = getClientName(newsockfd);
        if (!checkClientAcceptance(newsockfd, message)) continue;
        newsockfd2 = accept(sockfd2, (struct sockaddr *) &cli_addr, &clilen);
        newsockfd3 = accept(sockfd3, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd == -1)
            fprintf(stderr, "ERROR on accept\n");
        else {
            printf("Connection established successfully.\n\n");
            pthread_t th1, th2, th3;
            ThreadArgs* args = new ThreadArgs;
            args->socket = newsockfd2;
            args->message = message.client;
            ThreadArgs* args2 = new ThreadArgs;
            args2->socket = newsockfd3;
            args2->message = message.client;
            pthread_create(&th3, nullptr, inotify_thread, args);
            pthread_create(&th2, nullptr, listener_thread, args2);
            pthread_create(&th1, nullptr, client_thread, &newsockfd);
        }
    }
    return 0;
}

void *client_thread(void *arg) {
    MESSAGE message;
    int sockfd = *(int *) arg;
    int running = 1;
    ssize_t n;

    while (running) {
        do {
            n = read(sockfd, (void *) &message, sizeof(message));
        } while (n < sizeof(message));

        if (n < 0) {
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

int create_connection(int port) {
    int sockfd, bindReturn;
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
    bindReturn = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (bindReturn < 0) {
        fprintf(stderr, "ERROR on binding\n");
        exit(-1);
    }
    return sockfd;
}

MESSAGE getClientName(int sockfd){
    MESSAGE message;
    ssize_t n;

    do {
        n = read(sockfd, (void *) &message, sizeof(message));
    } while (n < sizeof(message));

    return message;
}

bool checkClientAcceptance(int sockfd, MESSAGE message) {
    bool accepted = true;
    ssize_t n;

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

    n = write(sockfd, (void *) &message, sizeof(MESSAGE));
    if (n < 0) {
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



