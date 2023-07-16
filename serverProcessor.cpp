#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sstream>
#include <vector>
#include <string>

#include "./utils.hpp"

using namespace std;

class ServerProcessor {
private:
    int socket{};
    MESSAGE message;
    vector<string> splitCommand{};

    void handleUpload() {
        printf("Upload command selected.\n");

        int size;
        if (!receiveAll(socket, (void *) &size, sizeof(int))) {
            fprintf(stderr, "ERROR receiving file size\n");
            return;
        }
        printf("File size: %d\n", size);

        if (size == -1) {
            fprintf(stderr, "ERROR: file not found\n");
            return;
        }

        string location = "sync_dir_" + string(message.client) + "/" + splitCommand[1];

        printf("Location: %s\n", location.c_str());

        FILE *file = fopen(location.c_str(), "wb");
        if (!file) {
            fprintf(stderr, "Error creating file\n");
            return;
        }

        const int BUFFER_SIZE = 1024;
        char buffer[BUFFER_SIZE];
        ssize_t bytesRead, totalBytesReceived = 0;

        while (totalBytesReceived < (size - 1)) {
            bytesRead = read(socket, buffer, BUFFER_SIZE - 1);
            if (bytesRead <= 0) {
                fprintf(stderr, "Error receiving file data\n");
                fclose(file);
                return;
            }

            buffer[bytesRead] = '\0';

            fwrite(buffer, bytesRead, 1, file);
            totalBytesReceived += bytesRead;
        }

        printf("Upload successful\n");
        fclose(file);

    }

    void handleDownload() {
        printf("Download command selected.\n");
        char location[256] = "sync_dir_";
        strcat(location, message.client);
        strcat(location, "/");
        strcat(location, splitCommand[1].c_str());

        printf("Location: %s\n", location);

        int size = -1;
        FILE *file = fopen(location, "rb");
        if (!file) {
            if (!sendAll(socket, (void *) &size, sizeof(ssize_t))) {
                fprintf(stderr, "ERROR: fail to send not existent file message\n");
            }
            fprintf(stderr, "ERROR not existent file message\n");
            fclose(file);
            return;
        }

        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fseek(file, 0, SEEK_SET);

        printf("Size: %d\n", size);

        if (!sendAll(socket, (void *) &size, sizeof(int))) {
            fprintf(stderr, "ERROR sending file size\n");
            fclose(file);
            return;
        }

        if (size == 0) {
            fclose(file);
            return;
        }

        const int BUFFER_SIZE = 1024;
        char buffer[BUFFER_SIZE];
        size_t bytesRead;
        size_t totalBytesSent = 0;

        while ((bytesRead = fread(buffer, 1, BUFFER_SIZE - 1, file)) > 0) {
            if (!sendAll(socket, (void *) buffer, bytesRead)) {
                fprintf(stderr, "Error sending file data\n");
                fclose(file);
                return;
            }
            totalBytesSent += bytesRead;
        }

        printf("Download successful.\n");
        fclose(file);
    }

    void handleDelete() {
        char returnMessage[MAX_MESSAGE_LENGTH + 1];
        printf("Delete command selected.\n");
        printf("Client name: %s\n", message.client);

        char location[256] = "sync_dir_"; // Declare 'location' as an array of characters
        strcat(location, message.client);
        printf("Location: %s\n", location);

        DIR *d;
        struct dirent *dir;
        d = opendir(location);

        if (d) {
            while ((dir = readdir(d))) {
                if (dir->d_name != splitCommand[1]) continue;

                strcat(location, "/");
                strcat(location, dir->d_name);
                remove(location);

                printf("Successfully deleted file: %s\n\n", splitCommand[1].c_str());
                snprintf(returnMessage, sizeof(returnMessage), "%s deleted\n", splitCommand[1].c_str());
                if (!sendAll(socket, returnMessage, MAX_MESSAGE_LENGTH)) {
                    fprintf(stderr, "Error sending delete confirmation\n");
                }
                closedir(d);
                return;
            }
            printf("Fail to delete file [%s]: not found\n\n", splitCommand[1].c_str());
            closedir(d);
        }
        snprintf(returnMessage, sizeof(returnMessage), "Failed to delete %s \n", splitCommand[1].c_str());
        if (!sendAll(socket, returnMessage, MAX_MESSAGE_LENGTH)) {
            fprintf(stderr, "Error sending delete fail message\n");
        }
    }

    void handleLs() {
        printf("LS command selected!\n");
        printf("Client name: %s\n", message.client);

        char location[256] = "sync_dir_"; // Declare 'location' as an array of characters
        strcat(location, message.client);
        printf("Location: %s\n", location);

        DIR *d;
        struct dirent *dir;
        d = opendir(location);
        char directoryNames[50][256]; // Array to store directory names
        int count = 0; // Count of directory names

        if (d) {
            while ((dir = readdir(d))) {
                if (strcmp(dir->d_name, ".") != 0 && (strcmp(dir->d_name, "..") != 0)) {
                    strcpy(directoryNames[count], strcat(dir->d_name, "\n"));
                    count++;
                }
            }
            closedir(d);
            strcpy(directoryNames[count], "");

            if (!sendAll(socket, directoryNames, 12800)) {
                fprintf(stderr, "ERROR sending ls command result\n");
            }
        }
    }

    void handleLc() {
        printf("LC command selected.\n");
        // Your lc code here
    }

    void handleGsd() {
        printf("GSD command selected.\n");
        // Your gsd code here
    }

public:
    ServerProcessor(int socket, MESSAGE message) : socket(socket), message(message) {
        splitCommand = ServerProcessor::splitString(message.content);
    }

    int handleInput() {
        message.content[strcspn(message.content, "\n")] = 0;

        splitCommand = splitString(message.content);
        const string &mainCommand = splitCommand[0];

        if (mainCommand == "upload") {
            handleUpload();
        } else if (mainCommand == "download") {
            handleDownload();
        } else if (mainCommand == "delete") {
            handleDelete();
        } else if (mainCommand == "ls") {
            handleLs();
        } else if (mainCommand == "lc") {
            handleLc();
        } else if (mainCommand == "gsd") {
            handleGsd();
        } else if (mainCommand == "exit") {
            printf("Exiting the program.\n");
        } else {
            printf("Invalid content.\n\n");
        }

        return 0;
    }

private:
    static vector<string> splitString(const string &str) {
        string s;

        stringstream ss(str);

        vector<string> v;
        while (getline(ss, s, ' ')) {
            if (s != " ") {
                v.push_back(s);
            }
        }

        return v;
    }
};

