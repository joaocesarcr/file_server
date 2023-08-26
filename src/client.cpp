#include "../include/client.hpp"

int mainSocket;
int listenerSocket;
int monitorSocket;
int PORT;

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "ERROR incorrect usage\n");
        exit(-1);
    }

    PORT = stoi(argv[3]);

    mainSocket = createConnection(argv, PORT);
    if (!checkConnectionAcceptance(argv[1], mainSocket)) exit(-1);
    printf("Connection established successfully.\n\n");
    createSyncDir(argv[1]);

    listenerSocket = createConnection(argv, PORT + 1);
    monitorSocket = createConnection(argv, PORT + 2);

    pthread_t threadListener, threadMonitor, serverChangeListener;;
    auto *listenerArgs = new ThreadArgs{listenerSocket, argv[1]};
    auto *monitorArgs = new ThreadArgs{monitorSocket, argv[1]};

    pthread_create(&threadListener, nullptr, reinterpret_cast<void *(*)(void *)>(syncChanges), listenerArgs);
    pthread_create(&threadMonitor, nullptr, reinterpret_cast<void *(*)(void *)>(monitorSyncDir), monitorArgs);
    pthread_create(&serverChangeListener, nullptr, listenForServerChanges, nullptr);


    makeSyncDir(argv[1], mainSocket);

    MESSAGE message;
    strncpy(message.client, argv[1], MAX_MESSAGE_LENGTH);
    int running = 1;
    do {
        string temp;
        getline(cin, temp);
        strcpy(message.content, temp.c_str());

        if (!strcmp(message.content, "exit")) {
            running = 0;
        }

        if (!sendAll(mainSocket, &message, sizeof(MESSAGE)))
            fprintf(stderr, "ERROR writing to socket\n");

        ClientProcessor handler = *new ClientProcessor(mainSocket, message);
        handler.handleInput();
    } while (running);
    printf("Ending connection\n");

    close(mainSocket);
    return 0;
}

int createConnection(char *argv[], int port) {
    int newSocket;
    struct sockaddr_in serv_addr{};
    struct hostent *server;

    server = gethostbyname(argv[2]);
    if (!server) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(-1);
    }

    newSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (newSocket == -1) {
        fprintf(stderr, "ERROR opening socket\n");
        exit(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = *((struct in_addr *) server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

    if (connect(newSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR establishing connection. Exiting...\n");
        exit(-1);
    }

    return newSocket;
}

bool checkConnectionAcceptance(char clientName[], int socket) {
    MESSAGE message;
    strcpy(message.client, clientName);
    string threadArgName = message.client;

    if (!sendAll(socket, (void *) &message, sizeof(MESSAGE))) {
        fprintf(stderr, "ERROR writing to socket\n");
    }

    if (!receiveAll(socket, (void *) &message, sizeof(MESSAGE))) {
        fprintf(stderr, "ERROR reading from socket\n");
    }

    if (strcmp(message.content, "accepted\0") == 0) {
        return true;
    }

    fprintf(stderr, "ERROR: Connections quota reached\n");

    close(socket);
    return false;
}

void makeSyncDir(char clientName[], int socket) {
    MESSAGE message;
    strncpy(message.client, clientName, MAX_MESSAGE_LENGTH);
    strncpy(message.content, "ls", MAX_MESSAGE_LENGTH);

    if (!sendAll(socket, &message, sizeof(MESSAGE)))
        fprintf(stderr, "ERROR writing to socket\n");

    string strClientName = string(clientName);

    ClientProcessor handler = *new ClientProcessor(socket, message);
    handler.handleGsd();
}

int setupListenSocket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR opening socket\n");
        exit(-1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);  // Change to your preferred port
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR on binding\n");
        exit(-1);
    }

    listen(sockfd, 5);

    return sockfd;
}

void changeConnectionsToNewHost(char *newHost) {

    struct hostent *server = gethostbyname(newHost);
    if (!server) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(-1);
    }

    mainSocket = createConnection(&newHost, PORT);
    listenerSocket = createConnection(&newHost, PORT + 1);
    monitorSocket = createConnection(&newHost, PORT + 2);

}

void *listenForServerChanges(void *arg) {
    int listenSocket;
    struct sockaddr_in serv_addr{};

    listenSocket = setupListenSocket();  
    
    while(true) {
        socklen_t socketSize = sizeof(struct sockaddr_in);
        int updateSocket = accept(listenSocket, (struct sockaddr *) &serv_addr, &socketSize);
        char newHost[MAX_MESSAGE_LENGTH];
        receiveAll(updateSocket, newHost, MAX_MESSAGE_LENGTH);
        
        changeConnectionsToNewHost(newHost);
        
        close(updateSocket);
    }
    return nullptr;
}