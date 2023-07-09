#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sstream>
#include <vector>
#include <string>

#include <sys/stat.h>
#include "./h/message_struct.hpp"

using namespace std;

class ClientProcessor {
private:
    int socket{};
    MESSAGE message;
    vector<string> splitCommand{};


public:
    ClientProcessor(int socket, MESSAGE message) : socket(socket), message(message) {
        splitCommand = ClientProcessor::splitString(message.content);
    }

    void handleUpload() {
        if (splitCommand.size() < 2) {
            fprintf(stderr, "ERROR: insufficient arguments\n");
            return;
        }

        size_t n;
        FILE *file = fopen(splitCommand[1].c_str(), "rb");
        if (file) {
            string fileName = splitCommand[1].substr(splitCommand[1].find_last_of("/") + 1);
            string serverFilePath = "server_files/" + string(message.client) + "/" + fileName;

            fseek(file, 0, SEEK_END);
            ssize_t fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            n = write(socket, (void *) &fileSize, sizeof(ssize_t));
            if (n <= 0) {
                fprintf(stderr, "Error sending file size\n");
                fclose(file);
                return;
            }

            const int BUFFER_SIZE = 1024;
            char buffer[BUFFER_SIZE];
            ssize_t bytesRead;
            ssize_t totalBytesSent = 0;

            while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                n = write(socket, buffer, bytesRead);
                if (n <= 0) {
                    fprintf(stderr, "Error sending file data\n");
                    fclose(file);
                    break;
                }

                totalBytesSent += bytesRead;
            }

            printf("Total bytes sent: %zd\n", totalBytesSent);
            fclose(file);
        }
    }

    void handleDownload() {
        ssize_t n;
        long size = 0;
        printf("Getting file size...\n");
        n = read(socket, (void *) &size, sizeof(long));
        printf("File size: %ld\n", size);

        FILE *file;
        char *buffer = new char[size + 1];
        // TODO: colocar o argumento
        file = fopen(splitCommand[1].c_str(), "wb+");
        if (!file) {
            fprintf(stderr, "Error opening file");
        }

        ssize_t bytesRead;

        n = read(socket, (void *) buffer, size);
        fwrite(buffer, size, 1, file);
        delete[] buffer;
        fclose(file);
    }

    void handleDelete() {

    }

    void handleLs() {
        size_t n;
        char directoryNames[50][256];
        do {
            n = read(socket, directoryNames, 12800);
        } while (n < sizeof(message));
        for (auto &directoryName: directoryNames) {
            if (!strcmp(directoryName, ""))
                break;
            printf("%s", directoryName);
        }
    }

    void handleLc() {
        printf("LC command selected!\n");
        printf("Client name: %s\n", message.client);

        char location[256] = "sync_dir_/"; // Declare 'location' as an array of characters
        strcat(location, message.client);
        printf("Location: %s\n", location);

        DIR *d;
        struct dirent *dir;
        d = opendir(location);

        if (d) {
            while ((dir = readdir(d))) {
                if (strcmp(dir->d_name, ".") != 0 && (strcmp(dir->d_name, "..") != 0)) {
                    printf("%s\n", dir->d_name);
                }
            }
            closedir(d);
        }
    }

    void handleGsd() {
        printf("GSD content selected.\n");
        // Your gsd code here
    }

    int handleInput() {
        // Remove \n
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