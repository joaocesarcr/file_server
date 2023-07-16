#include "utils.hpp"

bool shoudNotify = true;

pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_file_update = PTHREAD_MUTEX_INITIALIZER;

bool lock_change = false;

bool receiveAll(int socket, void *buffer, size_t length) {
    char *data = static_cast<char *>(buffer);
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

bool sendAll(int socket, const void *buffer, size_t length) {
    const char *data = static_cast<const char *>(buffer);
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

void *monitor_sync_dir_folder(void *arg) {
    int inotifyFd = inotify_init();
    if (inotifyFd == -1) {
        std::cerr << "Failed to initialize inotify" << std::endl;
        return nullptr;
    }

    ThreadArgs args = *(ThreadArgs *) arg;

    string clientName = args.message;

    int socket = args.socket;

    filesystem::path currentPath = std::filesystem::current_path();
    string filename = "sync_dir_" + clientName;
    filesystem::path absolutePath = currentPath / filename;
    string absolutePathString = absolutePath.string();

    int watchDescriptor = inotify_add_watch(inotifyFd, absolutePathString.c_str(),
                                            IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_CLOSE_WRITE);
    if (watchDescriptor == -1) {
        std::cerr << "Failed to add watch to the folder" << std::endl;
        return nullptr;
    }

    char buffer[4096];
    while (true) {
        ssize_t bytesRead = read(inotifyFd, buffer, sizeof(buffer));
        if (bytesRead == -1) {
            std::cerr << "Failed to read from inotify" << std::endl;
            break;
        }

        ssize_t offset = 0;
        while (offset < bytesRead) {
            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(&buffer[offset]);

            if (event->len > 0) {
                std::string filename(event->name);
                MESSAGE message;
                pthread_mutex_lock(&mutex_file_update);
                if(lock_change){
                    lock_change = false;
                    pthread_mutex_unlock(&mutex_file_update);
                    continue;
                }
                lock_change = true;
                pthread_mutex_unlock(&mutex_file_update);
                switch (event->mask) {
                    case IN_DELETE:
                    case IN_MOVED_FROM:                           
                        strcpy(message.client, filename.c_str());
                        strcpy(message.content, "movout");
                        std::cout << "File deleted: " << filename << std::endl;
                        if (!sendAll(socket, &message, sizeof(MESSAGE))) fprintf(stderr, "ERROR writing to socket\n");
                        break;
                    case IN_CREATE:  
                    case IN_CLOSE_WRITE:   
                    case IN_MOVED_TO:                   
                        strcpy(message.client, filename.c_str());
                        strcpy(message.content, "create");
                        std::cout << "File created: " << filename << std::endl;
                        if (!sendAll(socket, &message, sizeof(MESSAGE))) { 
                            fprintf(stderr, "ERROR writing to socket\n");
                            break; 
                        }
                        int fileSize;
                        
                        string location = absolutePathString + '/' + filename;
                        cout << location << endl;
                        FILE *file = fopen(location.c_str(), "rb");
                        
                        fseek(file, 0, SEEK_END);
                        
                        fileSize = ftell(file);
                        
                        fseek(file, 0, SEEK_SET);
                        
                        if (!sendAll(socket, (void *) &fileSize, sizeof(int))) {
                            fprintf(stderr, "Error sending file size\n");
                            fclose(file);
                            break;
                        }
                        
                        if (fileSize < 0) {
                            fclose(file);
                            break;
                        }
                        const int BUFFER_SIZE = 1024;
                        char buffer[BUFFER_SIZE];
                        size_t bytesRead;
                        size_t totalBytesSent = 0;

                        while ((bytesRead = fread(buffer, 1, BUFFER_SIZE - 1, file)) > 0) {
                            if (!sendAll(socket, (void *) buffer, bytesRead)) {
                                fprintf(stderr, "Error sending file data\n");
                                fclose(file);
                                break;
                            }

                            totalBytesSent += bytesRead;
                        }
                        fclose(file);
                        cout << "finished synching";
                        break;
                }
            }

            offset += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(inotifyFd, watchDescriptor);
    close(inotifyFd);
    return nullptr;
}

void *listenSocket(void* arg) {
    ThreadArgs* args = static_cast<ThreadArgs*>(arg);
    int socket = args->socket;
    MESSAGE message;

    string clientName = args->message;

    filesystem::path currentPath = std::filesystem::current_path();
    string filename = "sync_dir_" + clientName;
    filesystem::path absolutePath = currentPath / filename;
    string absolutePathString = absolutePath.string();

    char buffer[7];
    while (true) {
        if (!receiveAll(socket, (void *) &message, sizeof(message))) {
            fprintf(stderr, "ERROR reading from socket\n");
            return (void *) -1;
        }

        if (sizeof(message.content) <= 0 or sizeof(message.client) <= 0) {
            break;
        }

        if (!strcmp(message.content, "create")) {
            string filename = message.client;
            std::cout << "Received message: create " << filename << std::endl;
            int size;
            string location = absolutePathString + '/' + filename;
            if (!receiveAll(socket, (void *) &size, sizeof(int))) {
                fprintf(stderr, "ERROR receiving file size\n");
                return nullptr;
            }
            printf("File size: %d\n", size);

            FILE *file = fopen(location.c_str(), "wb");
            if (!file) {
                fprintf(stderr, "Error creating file\n");
                return nullptr;
            }

            const int BUFFER_SIZE = 1024;
            char buffer[BUFFER_SIZE];
            ssize_t bytesRead, totalBytesReceived = 0;

            while (totalBytesReceived < (size - 1)) {
                bytesRead = read(socket, buffer, BUFFER_SIZE - 1);
                if (bytesRead <= 0) {
                    fprintf(stderr, "Error receiving file data\n");
                    fclose(file);
                    return nullptr;
                }

                buffer[bytesRead] = '\0';

                fwrite(buffer, bytesRead, 1, file);
                totalBytesReceived += bytesRead;
            }
            fclose(file);
            continue;
        } else if (!strcmp(message.content, "movout")) {
            std::cout << "Received message: delete" << std::endl;
            string filename = message.client;
            string location = absolutePathString + '/' + filename;
            cout << location << endl;
            // Check if the file exists
            if (std::remove(location.c_str()) != 0) {
                printf("Error deleting the file.\n");
            } else {
                printf("File deleted successfully.\n");
            }
        }
    }

    close(socket);
    return nullptr;
}

void *inotify_thread(void *arg) {
    int length, i = 0, wd;
    int fd;
    char buffer[BUF_LEN];

    /* Initialize Inotify*/
    fd = inotify_init();
    if (fd < 0) {
        perror("Couldn't initialize inotify");
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

    if (wd == -1) {
        cout << "Couldn't add watch to" << absolutePathString << endl;
    } else {
        cout << "Watching: " << absolutePathString << endl;
    }

    while (true) {
        i = 0;
        if (!receiveAll(fd, buffer, BUF_LEN)) {
            perror("read");
        }

        while (i < length) {
            auto *event = (struct inotify_event *) &buffer[i];
            if (event->len) {
                pthread_mutex_lock(&mutex2);
                if (!shoudNotify) {
                    shoudNotify = true;
                    continue;
                }
                shoudNotify = false;
                pthread_mutex_unlock(&mutex2);
                if ((event->mask & IN_CREATE) || (event->mask & IN_MODIFY) || (event->mask & IN_CLOSE_WRITE)) {
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
                        exit(-1);
                    }

                    fseek(file, 0, SEEK_END);
                    long size = ftell(file);
                    fseek(file, 0, SEEK_SET);

                    if (!sendAll(sockfd, (void *) &size, sizeof(long))) {
                        fprintf(stderr, "Error sending file size\n");
                        fclose(file);
                        exit(-1);
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
                            exit(-1);
                        }
                        totalBytesSent += bytesRead;
                    }

                    cout << "total: " << totalBytesSent << endl;
                    fclose(file);
                }
                if (event->mask & IN_DELETE) {
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
    inotify_rm_watch(fd, wd);
    close(fd);
    close(args.socket);
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
            cout << "total: " << totalBytesReceived << endl;
            fclose(file);
            clock_t start_time = clock();
            while ((clock() - start_time) / CLOCKS_PER_SEC < 10);

        } else if (!strcmp(message.content, "delete")) {
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

void createSyncDir(const string &clientName) {
    string dirPath = "sync_dir_" + clientName;

    mkdir(dirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

