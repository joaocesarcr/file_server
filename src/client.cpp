#include "../include/client.hpp"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "ERROR incorrect usage\n");
        exit(-1);
    }

    const int PORT = stoi(argv[3]);

    int mainSocket = createConnection(argv, PORT);
    if (!checkConnectionAcceptance(argv[1], mainSocket)) exit(-1);
    printf("Connection established successfully.\n\n");
    createSyncDir(argv[1]);

    int listenerSocket = createConnection(argv, PORT + 1);
    int monitorSocket = createConnection(argv, PORT + 2);

    pthread_t threadListener, threadMonitor;
    auto *listenerArgs = new ThreadArgs{listenerSocket, argv[1]};
    auto *monitorArgs = new ThreadArgs{monitorSocket, argv[1]};

    pthread_create(&threadListener, nullptr, reinterpret_cast<void *(*)(void *)>(syncChanges), listenerArgs);
    pthread_create(&threadMonitor, nullptr, reinterpret_cast<void *(*)(void *)>(monitorSyncDir), monitorArgs);

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