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

int create_connection(int port);

bool checkClientAcceptance(int sockfd, MESSAGE message);

void removeClientConnectionsCount(int sockfd);

void createSyncDir(const string& clientName);

void *inotify_thread(void *arg);

void *listener_thread(void *arg);

MESSAGE getClientName(int sockfd);

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
            pthread_t th1;
            ThreadArgs* args = new ThreadArgs;
            args->socket = newsockfd2;
            args->message = message.client;
            ThreadArgs* args2 = new ThreadArgs;
            args2->socket = newsockfd3;
            args2->message = message.client;
            pthread_t th2;
            pthread_create(&th2, nullptr, inotify_thread, args);
            pthread_create(&th2, nullptr, listener_thread, args2);
            pthread_create(&th1, nullptr, client_thread, &newsockfd);
            socketList.push_back(newsockfd2);
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
                        MESSAGE message;
                        strcpy(message.client, clientName.c_str());
                        strcpy(message.content, "create");
                        if (!sendAll(sockfd, &message, sizeof(MESSAGE))) fprintf(stderr, "ERROR writing to socket\n");  

                        char location[256] = "sync_dir_";
                        strcat(location, message.client);
                        int n;
                        strcat(location, "/");
                        strcat(location, event->name);
                        if (!sendAll(sockfd, &event->name, sizeof(char[100]))) fprintf(stderr, "ERROR writing to socket\n"); 
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
    cout << sockfd << endl;

    while (running) {
        do {
            n = read(sockfd, (void *) &message, sizeof(message));
        } while (n < sizeof(message));

        if (n < 0) {
            fprintf(stderr, "ERROR reading from socket\n");
            return (void *) -1;
        }
        if (!strcmp(message.content, "create")) {
            cout << message.content << endl;
            string name;
            n = receiveAll(sockfd, (void *) &name, sizeof(char[32]));
            cout << name << endl;
            string path = absolutePathString + name;
            cout << path << endl;
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