#include <pthread.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <limits.h> 
#include <filesystem>
#include <iostream>

#include "./serverProcessor.cpp"

#define PORT 4000
#define MAX_CONNECTIONS_PER_CLIENT 2
#define MAX_TOTAL_CONNECTIONS 5
 
#define MAX_EVENTS 1024 /*Max. number of events to process at one go*/
#define LEN_NAME 16 /*Assuming that the length of the filename won't exceed 16 bytes*/
#define EVENT_SIZE  ( sizeof (struct inotify_event) ) /*size of one event*/
#define BUF_LEN     ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )) /*buffer to store the data of events*/

void *client_thread(void *arg);

int create_connection();

bool checkClientAcceptance(int sockfd);

void removeClientConnectionsCount(int sockfd);

void createSyncDir(const string& clientName);

void *inotify_thread(void *arg);

bool sendAll(int socket, const void* buffer, size_t length);

map<string, int> clientsActiveConnections{};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

vector<int> socketList;
pthread_mutex_t mutex_socket_list = PTHREAD_MUTEX_INITIALIZER;

struct ThreadArgs {
    int socket;
    string message;
};

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
            pthread_mutex_lock(&mutex);  
            pthread_create(&th1, nullptr, client_thread, &newsockfd);
            socketList.push_back(newsockfd);
            pthread_mutex_unlock(&mutex);  
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
    ssize_t n;

    do {
        n = read(sockfd, (void *) &message, sizeof(message));
    } while (n < sizeof(message));

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
        ThreadArgs* args = new ThreadArgs;
        args->socket = sockfd;
        args->message = clientName;
        createSyncDir(clientName);
        pthread_t th2;
        pthread_create(&th2, nullptr, inotify_thread, args);
    }

    return accepted;
}

bool receiveAll(int socket, void* buffer, size_t length) {
    char* data = static_cast<char*>(buffer);
    ssize_t totalReceived = 0;

    while (totalReceived < length) {
        ssize_t received = read(socket, data + totalReceived, length - totalReceived);

        if (received < 1) {
            return false;
        }

        totalReceived += received;
    }

    return true;
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
                if ( event->mask & IN_CREATE) {
                    pthread_mutex_lock(&mutex);  
                        for (int socket : socketList) {
                            MESSAGE message;
                            strcpy(message.client, clientName.c_str());
                            strcpy(message.content, event->name);
                            if (!sendAll(socket, &message, sizeof(MESSAGE))) fprintf(stderr, "ERROR writing to socket\n");  

                            char location[256] = "sync_dir_";
                            strcat(location, message.client);
                            int n;
                            strcat(location, "/");
                            strcat(location, event->name);

                            printf("Location: %s\n", location);

                            FILE *file = fopen(location, "rb");
                            if (!file) {
                                fprintf(stderr, "Error opening file\n");
                                return;
                            }

                            fseek(file, 0, SEEK_END);
                            long size = ftell(file);
                            fseek(file, 0, SEEK_SET);

                            printf("Size: %ld\n", size);

                            n = write(socket, (void *) &size, sizeof(long));
                            if (n <= 0) {
                                fprintf(stderr, "Error sending file size\n");
                                fclose(file);
                                return;
                            }

                            const int BUFFER_SIZE = 1024;
                            char buffer[BUFFER_SIZE];
                            size_t bytesRead;
                            long totalBytesSent = 0;

                            while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                                n = write(socket, buffer, bytesRead);
                                if (n <= 0) {
                                    fprintf(stderr, "Error sending file data\n");
                                    fclose(file);
                                    return;
                                }
                                totalBytesSent += bytesRead;
                            }

                            printf("Total bytes sent: %ld\n", totalBytesSent);
                            fclose(file);
                        }     
                    pthread_mutex_unlock(&mutex);
                }
                
                if ( event->mask & IN_MODIFY) {
                    printf( "%s was modified\n", event->name);       
                }
                
                if ( event->mask & IN_DELETE) {
                    printf( "%s was deleted\n", event->name);       
                }  

                i += EVENT_SIZE + event->len;
            }
        }
    }
    /* Clean up*/
    inotify_rm_watch( fd, wd );
    close( fd );

    return nullptr;
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