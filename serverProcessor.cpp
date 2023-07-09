#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sstream>
#include <utility>
#include <vector>
#include <string>

#include <sys/stat.h>
#include "./h/message_struct.hpp"

using namespace std;

class ServerProcessor {
private:
    int socket{};
    MESSAGE message;
    vector<string> splitCommand{};

    void handleUpload() {
        printf("Upload content selected.\n");
        char location[256] = "server_files/";
        strcat(location, message.client);
        int n;
        string filePath = splitCommand[1];
        string fileName = filePath.substr(filePath.find_last_of('/') + 1);
        strcat(location, "/");
        strcat(location, fileName.c_str());

        printf("Location: %s\n", location);

        FILE *file = fopen(location, "wb");
        if (!file) {
            printf("Error creating file\n");
            return;
        }

        ssize_t size;
        n = read(socket, (void *) &size, sizeof(ssize_t));
        if (n <= 0) {
            printf("Error receiving file size\n");
            fclose(file);
            return;
        }

        const int BUFFER_SIZE = 1024;
        char buffer[BUFFER_SIZE];
        ssize_t bytesRead, totalBytesReceived = 0;

        while (totalBytesReceived < size) {
            bytesRead = read(socket, buffer, BUFFER_SIZE);
            if (bytesRead <= 0) {
                printf("Error receiving file data\n");
                fclose(file);
                return;
            }

            fwrite(buffer, bytesRead, 1, file);
            totalBytesReceived += bytesRead;
        }

        printf("Total bytes received: %zd\n", totalBytesReceived);
        fclose(file);
    }

    void handleDownload() {
        printf("Download content selected.\n");
        char location[256] = "server_files/";
        strcat(location, message.client);
        int n;
        strcat(location, "/");
        strcat(location, splitCommand[1].c_str());

        printf("Location: %s\n", location);

        FILE *file = fopen(location, "rb");
        if (!file) {
            printf("Error opening file\n");
            return;
        }

        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);

        printf("Size: %ld\n", size);

        n = write(socket, (void *) &size, sizeof(long));
        if (n <= 0) {
            printf("Error sending file size\n");
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
                printf("Error sending file data\n");
                fclose(file);
                return;
            }
            totalBytesSent += bytesRead;
        }

        printf("Total bytes sent: %ld\n", totalBytesSent);
        fclose(file);
   }

    void handleDelete() {
        char returnMessage[MAX_MESSAGE_LENGTH + 1];
        int n = 0;
        printf("Delete content selected.\n");
        printf("Client name: %s\n", message.client);

        char location[256] = "server_files/"; // Declare 'location' as an array of characters
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
                n = write(socket, returnMessage, MAX_MESSAGE_LENGTH);
                closedir(d);
                return;
            }
            printf("Fail to delete file [%s]: not found\n\n", splitCommand[1].c_str());
            closedir(d);
        }
        snprintf(returnMessage, sizeof(returnMessage), "Failed to delete %s \n", splitCommand[1].c_str());
        n = write(socket, returnMessage, MAX_MESSAGE_LENGTH);
    }

    void handleLs() {
        printf("LS content selected!\n");
        printf("Client name: %s\n", message.client);

        char location[256] = "server_files/"; // Declare 'location' as an array of characters
        strcat(location, message.client);
        printf("Location: %s\n", location);

        DIR *d;
        struct dirent *dir;
        d = opendir(location);
        char directoryNames[50][256]; // Array to store directory names
        int n, count = 0; // Count of directory names

        if (d) {
            while ((dir = readdir(d))) {
                if (strcmp(dir->d_name, ".") != 0 && (strcmp(dir->d_name, "..") != 0)) {
                    strcpy(directoryNames[count], strcat(dir->d_name, "\n"));
                    count++;
                }
            }
            closedir(d);
            strcpy(directoryNames[count], "");
            n = write(socket, directoryNames, 12800);
        }
    }

    void handleLc() {
        printf("LC content selected.\n");
        // Your lc code here
    }

    void handleGsd() {
        printf("GSD content selected.\n");
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
