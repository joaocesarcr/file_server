#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <filesystem>

#include "./h/message_struct.hpp"
#include "./clientProcessor.cpp"

#define PORT 4000
#define MAX_EVENTS 1024 /*Max. number of events to process at one go*/
#define LEN_NAME 16 /*Assuming that the length of the filename won't exceed 16 bytes*/
#define EVENT_SIZE  ( sizeof (struct inotify_event) ) /*size of one event*/
#define BUF_LEN     ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )) /*buffer to store the data of events*/

using namespace std;

struct ThreadArgs {
    int socket;
    string message;
};

int createConnection(char *argv[], int port);

bool checkConnectionAcceptance(char clientName[], int socket);

void createSyncDir(const string& clientName);

bool sendAll(int socket, const void* buffer, size_t length);

void *inotify_thread(void *arg);

void *listener_thread(void *arg);

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

        n = write(sockfd, (void *) &message, sizeof(MESSAGE));
        if (n < 0)
            fprintf(stderr, "ERROR writing to socket\n");

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

    n = write(socket, (void *) &message, sizeof(MESSAGE));
    if (n < 0)
        fprintf(stderr, "ERROR writing to socket\n");

    do {
        n = read(socket, (void *) &message, sizeof(MESSAGE));
    } while (n < sizeof(MESSAGE));

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

bool sendAll(int socket, const void* buffer, size_t length) {
    const char* data = static_cast<const char*>(buffer);
    ssize_t totalSent = 0;

    while (totalSent < length) {
        ssize_t sent = write(socket, data + totalSent, length - totalSent);

        if (sent == -1) {
            return false;
        }

        totalSent += sent;
    }

    return true;
}

void *inotify_thread(void *arg) {
    int length, i = 0, wd;
    int fd;
    char buffer[BUF_LEN];
    
    /* Initialize Inotify*/
    fd = inotify_init();
    if ( fd < 0 ) {
        perror( "Couldn't initialize inotify");
    }
    ThreadArgs args = *(ThreadArgs *) arg;

    string clientName = args.message;
    int sockfd = args.socket;

    filesystem::path currentPath = std::filesystem::current_path();
    string filename = "sync_dir_" + clientName;
    filesystem::path absolutePath = currentPath / filename;
    string absolutePathString = absolutePath.string();

    cout << "Absolute path: " << absolutePathString << std::endl;

    wd = inotify_add_watch(fd, absolutePathString.c_str(), IN_CREATE | IN_MODIFY | IN_DELETE); 
    
    if (wd == -1)
        {
        cout << "Couldn't add watch to"<< absolutePathString << endl;
        }
    else
        {
        cout << "Watching::" << absolutePathString << endl;
        }
 
    while(1)
    {
        i = 0;
        length = read( fd, buffer, BUF_LEN );  

        if ( length < 0 ) {
            perror( "read" );
        }  

        while ( i < length ) {
            struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
            if ( event->len ) {
                if ( (event->mask & IN_CREATE) or (event->mask & IN_MODIFY)) {
                        cout << "create" << endl;
                        MESSAGE message;
                        strcpy(message.client, clientName.c_str());
                        strcpy(message.content, "create");
                        if (!sendAll(sockfd, &message, sizeof(MESSAGE))) fprintf(stderr, "ERROR writing to socket\n");  

                        char location[256] = "sync_dir_";
                        strcat(location, message.client);
                        cout << sockfd << endl;
                        int n;
                        strcat(location, "/");
                        strcat(location, event->name);
                        cout << event->name <<endl;
                        if (!sendAll(sockfd, &event, sizeof(event))) fprintf(stderr, "ERROR writing to socket\n");
                        cout << location << endl; 
                        FILE *file = fopen(location, "rb");
                        if (!file) {
                            fprintf(stderr, "Error opening file\n");
                            exit;
                        }

                        fseek(file, 0, SEEK_END);
                        long size = ftell(file);
                        fseek(file, 0, SEEK_SET);

                        n = write(sockfd, (void *) &size, sizeof(long));
                        if (n <= 0) {
                            fprintf(stderr, "Error sending file size\n");
                            fclose(file);
                            exit;
                        }

                        const int BUFFER_SIZE = 1024;
                        char buffer[BUFFER_SIZE];
                        size_t bytesRead;
                        long totalBytesSent = 0;

                        while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                            n = write(sockfd, buffer, bytesRead);
                            if (n <= 0) {
                                fprintf(stderr, "Error sending file data\n");
                                fclose(file);
                                exit;
                            }
                            totalBytesSent += bytesRead;
                        }
                        fclose(file);    
                }
                if ( event->mask & IN_DELETE) {
                            MESSAGE message;
                            strcpy(message.client, clientName.c_str());
                            strcpy(message.content, "delete");
                            if (!sendAll(sockfd, &message, sizeof(MESSAGE))) fprintf(stderr, "ERROR writing to socket\n");
                            strcpy(message.client, clientName.c_str());
                            strcpy(message.content, event->name);
                            if (!sendAll(sockfd, &message, sizeof(MESSAGE))) fprintf(stderr, "ERROR writing to socket\n");      
                }  
                i += EVENT_SIZE + event->len;
            }
        }
    }
    inotify_rm_watch( fd, wd );
    close( fd );
    close( args.socket );
    return nullptr;
}

void *listener_thread(void *arg) {
    MESSAGE message;
    ThreadArgs args = *(ThreadArgs *) arg;

    string clientName = args.message;

    filesystem::path currentPath = std::filesystem::current_path();
    string filename = "sync_dir_" + clientName;
    filesystem::path absolutePath = currentPath / filename;
    string absolutePathString = absolutePath.string();
    absolutePathString = absolutePathString + "/";
    int sockfd = args.socket;
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
        if (!strcmp(message.content, "create")) {
            string name;
            n = read(sockfd, (void *) &name, sizeof(char[32]));
            string path = absolutePathString + name;
            FILE *file = fopen(path.c_str(), "wb");
            if (!file) {
                fprintf(stderr, "Error opening file\n");
                exit;
            }
            ssize_t size;
            n = read(sockfd, (void *) &size, sizeof(ssize_t));
            if (n <= 0) {
                fprintf(stderr, "Error receiving file size\n");
                fclose(file);
                break;
            }
            const int BUFFER_SIZE = 1024;
            char buffer[BUFFER_SIZE];
            ssize_t bytesRead, totalBytesReceived = 0;

            while (totalBytesReceived < size) {
                bytesRead = read(sockfd, buffer, BUFFER_SIZE);
                if (bytesRead <= 0) {
                    fprintf(stderr, "Error receiving file data\n");
                    fclose(file);
                    break;
                }

                fwrite(buffer, bytesRead, 1, file);
                totalBytesReceived += bytesRead;
            }
            fclose(file);
            
        } else if(!strcmp(message.content, "delete")) {
            n = read(sockfd, (void *) &message, sizeof(message));
            int removed = std::remove(message.content);
            if (removed != 0) {
                std::perror("Error deleting the file");
            }
        }
    }

    close(args.socket);

    return nullptr;
}