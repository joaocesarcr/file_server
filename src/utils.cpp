#include "utils.hpp"

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

void *monitorSyncDir(void *arg) {
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
            struct inotify_event *event = reinterpret_cast<struct inotify_event *>(&buffer[offset]);

            if (event->len > 0) {
                std::string filename(event->name);
                MESSAGE message;
                pthread_mutex_lock(&mutex_file_update);
                if (lock_change) {
                    lock_change = false;
                    pthread_mutex_unlock(&mutex_file_update);
                    break;
                }
                lock_change = true;
                pthread_mutex_unlock(&mutex_file_update);
                switch (event->mask) {
                    case IN_DELETE:
                    case IN_MOVED_FROM:
                        strcpy(message.client, filename.c_str());
                        strcpy(message.content, "movout");
                        if (!sendAll(socket, &message, sizeof(MESSAGE))) fprintf(stderr, "ERROR writing to socket\n");
                        break;
                    case IN_CREATE:
                    case IN_CLOSE_WRITE:
                    case IN_MOVED_TO:
                        strcpy(message.client, filename.c_str());
                        strcpy(message.content, "create");
                        if (!sendAll(socket, &message, sizeof(MESSAGE))) {
                            fprintf(stderr, "ERROR writing to socket\n");
                            break;
                        }
                        int fileSize;

                        string location = absolutePathString + "/" + filename;
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

void *syncChanges(void *arg) {
    ThreadArgs *args = static_cast<ThreadArgs *>(arg);
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
            return (void *) -1;
        }

        pthread_mutex_lock(&mutex_file_update);
        if (!strcmp(message.content, "create")) {
            string filename = message.client;
            int size;
            string location = absolutePathString + '/' + filename;
            if (!receiveAll(socket, (void *) &size, sizeof(int))) {
                fprintf(stderr, "ERROR receiving file size\n");
                return nullptr;
            }

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
        } else if (!strcmp(message.content, "movout")) {
            string filename = message.client;
            string location = absolutePathString + '/' + filename;
            // Check if the file exists
            if (std::remove(location.c_str()) != 0) {
                printf("Error deleting the file.\n");
            } else {
                printf("File deleted successfully.\n");
            }
        }
        pthread_mutex_unlock(&mutex_file_update);
    }
    close(socket);
    return nullptr;
}

void createSyncDir(const string &clientName) {
    string dirPath = "sync_dir_" + clientName;

    mkdir(dirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

