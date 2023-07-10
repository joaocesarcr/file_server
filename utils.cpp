#include "utils.hpp"

bool shoudNotify = false;

pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

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
        if ( !receiveAll( fd, buffer, BUF_LEN ) ) {
            perror( "read" );
        }  

        while ( i < length ) {
            struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
            if ( event->len ) {
                pthread_mutex_lock(&mutex2);
                if(shoudNotify == false){ shoudNotify = true; continue; }
                shoudNotify = false;
                pthread_mutex_unlock(&mutex2);
                if ( (event->mask & IN_CREATE) || (event->mask & IN_MODIFY) || (event->mask & IN_CLOSE_WRITE)) {
                        MESSAGE message;
                        strcpy(message.client, clientName.c_str());
                        strcpy(message.content, "create");
                        if (!sendAll(sockfd, &message, sizeof(MESSAGE))) fprintf(stderr, "ERROR writing to socket\n");  

                        char location[256] = "sync_dir_";
                        strcat(location, message.client);
                        int n;
                        strcat(location, "/");
                        strcat(location, event->name);
                        char eventName[LEN_NAME];
                        strcpy(eventName, event->name);
                        if (!sendAll(sockfd, &eventName, LEN_NAME)) fprintf(stderr, "ERROR writing to socket\n");
                        FILE *file = fopen(location, "rb");
                        if (!file) {
                            fprintf(stderr, "Error opening file\n");
                            exit;
                        }

                        fseek(file, 0, SEEK_END);
                        long size = ftell(file);
                        fseek(file, 0, SEEK_SET);

                        if (!sendAll(sockfd, (void *) &size, sizeof(long))) {
                            fprintf(stderr, "Error sending file size\n");
                            fclose(file);
                            exit;
                        }
                        cout << "size: " << size << endl;

                        const int BUFFER_SIZE = 1024;
                        char buffer[BUFFER_SIZE];
                        size_t bytesRead;
                        long totalBytesSent = 0;

                        while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                            if (!sendAll(sockfd, buffer, bytesRead)) {
                                fprintf(stderr, "Error sending file data\n");
                                fclose(file);
                                exit;
                            }
                            totalBytesSent += bytesRead;
                        }

                        cout << "total: " << totalBytesSent  <<  endl;
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
        if (!receiveAll(sockfd, (void *) &message, sizeof(message))) {
            fprintf(stderr, "ERROR reading from socket\n");
            return (void *) -1;
        }
        if (!strcmp(message.content, "create")) {
            char name[LEN_NAME];
            n = receiveAll(sockfd, (void *) &name, LEN_NAME);
            string stringName = name;
            string path = absolutePathString + stringName;
            FILE *file = fopen(path.c_str(), "wb");
            if (!file) {
                fprintf(stderr, "Error opening file\n");
                exit;
            }
            long size;
            n = receiveAll(sockfd, (void *) &size, sizeof(long));
            if (n <= 0) {
                fprintf(stderr, "Error receiving file size\n");
                fclose(file);
                break;
            }

            cout << "size: " << size << endl;

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
            cout << "total: " << totalBytesReceived <<  endl;
            fclose(file);
            clock_t start_time = clock();
            while ((clock() - start_time) / CLOCKS_PER_SEC < 10);
            
        } else if(!strcmp(message.content, "delete")) {
            n = receiveAll(sockfd, (void *) &message, sizeof(message));
            int removed = std::remove(message.content);
            if (removed != 0) {
                std::perror("Error deleting the file");
            }
        }
    }

    close(args.socket);

    return nullptr;
}

