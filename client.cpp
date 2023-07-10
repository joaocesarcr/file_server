#include "./client.hpp"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "usage %s hostname\n", argv[0]);
        exit(-1);
    }

    int sockfd = createConnection(argv, PORT);
    if (!checkConnectionAcceptance(argv[1], sockfd)) exit(-1);
    int sockfd2 = createConnection(argv, PORT + 1);
    int sockfd3 = createConnection(argv, PORT + 2);

    ThreadArgs* args = new ThreadArgs;
    args->socket = sockfd2;
    args->message = argv[1];
    pthread_t th;
    pthread_create(&th, nullptr, listener_thread, args);
    ThreadArgs* args2 = new ThreadArgs;
    args2->socket = sockfd3;
    args2->message = argv[1];
    pthread_t th2;
    pthread_create(&th2, nullptr, inotify_thread, args2);
    

    MESSAGE message;
    strncpy(message.client, argv[1], MAX_MESSAGE_LENGTH);
    int running = 1;
    do {
        ssize_t n;
        string temp;
        getline(cin, temp);
        strcpy(message.content, temp.c_str());

        if (!strcmp(message.content, "exit")) {
            if (!sendAll(sockfd, &message, sizeof(MESSAGE)))
                fprintf(stderr, "ERROR writing to socket\n");

            running = 0;
        }

        if (!sendAll(sockfd, (void *) &message, sizeof(MESSAGE))) {
            fprintf(stderr, "ERROR writing to socket\n");
        }

        ClientProcessor handler = *new ClientProcessor(sockfd, message);
        handler.handleInput();


    } while (running);
    printf("Ending connection\n");

    close(sockfd);
    return 0;
}

int createConnection(char *argv[], int port) {
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
        fprintf(stderr, "ERROR opening socket\n");
        exit(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = *((struct in_addr *) server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR establishing connection. Exiting...\n");
        exit(-1);
    }

    

    createSyncDir(argv[1]);
    printf("Connection established successfully.\n\n");
    return sockfd;

}

bool checkConnectionAcceptance(char clientName[], int socket) {
    ssize_t n;
    MESSAGE message;
    strcpy(message.client, clientName);
    string threadArgName = message.client;

    if (!sendAll(socket, (void *) &message, sizeof(MESSAGE))) {
        fprintf(stderr, "ERROR writing to socket\n");
    }

    if (!receiveAll(socket, (void *) &message, sizeof(MESSAGE))) {
        fprintf(stderr, "ERROR reading from socket\n");
    }

    if (strcmp(message.content, "accepted\0") == 0){

        return true;  
    } 

    fprintf(stderr, "ERROR: Connections quota reached\n");

    return false;
}

void createSyncDir(const string& clientName) {
    string syncDirPath = "sync_dir_" + clientName;


    mkdir(syncDirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}
