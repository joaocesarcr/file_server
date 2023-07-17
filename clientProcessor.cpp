#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>


#include "./utils.hpp"

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

private:

    void handleUpload() {
        if (splitCommand.size() < 2) {
            fprintf(stderr, "ERROR: insufficient arguments\n");
            return;
        }

        string fileName = splitCommand[1].substr(splitCommand[1].find_last_of('/') + 1);
        string clientFilePath = "sync_dir_" + string(message.client) + "/" + fileName;

        int fileSize = -1;
        FILE *file = fopen(splitCommand[1].c_str(), "rb");
        if (!file) {
            if (!sendAll(socket, (void *) &fileSize, sizeof(ssize_t))) {
                fprintf(stderr, "ERROR: fail to send not existent file message\n");
            }
            fprintf(stderr, "ERROR not existent file message\n");
            fclose(file);
            return;
        }

        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (!sendAll(socket, (void *) &fileSize, sizeof(int))) {
            fprintf(stderr, "Error sending file size\n");
            fclose(file);
            return;
        }

        if (fileSize == 0) {
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
                break;
            }

            totalBytesSent += bytesRead;
        }

        printf("Upload successful.\n");
        fclose(file);
    }

    void handleDownload(string fileName) {
        if (fileName.empty()) printf("Download command selected.\n");

        int size;
        if (!receiveAll(socket, (void *) &size, sizeof(int))) {
            fprintf(stderr, "ERROR receiving file size\n");
            return;
        }

        if (size == -1) {
            fprintf(stderr, "ERROR: file not found\n");
            return;
        }

        string filePath = splitCommand[1];

        if (!fileName.empty()) {
            fileName = "sync_dir_" + string(message.client) + "/" + fileName;
        }else{
            fileName = filePath.substr(filePath.find_last_of('/') + 1);
        }

        FILE *file = fopen(fileName.c_str(), "wb");
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

        if (fileName.empty()) printf("Download successful.\n");
        fclose(file);
    }

    void handleDelete() {
        string location = "sync_dir_" + string(message.client);

        DIR *d;
        struct dirent *dir;
        d = opendir(location.c_str());

        if (!d) {
            fprintf(stderr, "ERROR Failed to open directory.\n");
        } else {
            string fileToDelete = splitCommand[1];

            while ((dir = readdir(d))) {
                if (dir->d_name != fileToDelete) continue;

                string filePath = location + "/" + dir->d_name;
                if (remove(filePath.c_str()) == 0) {
                    printf("Successfully deleted file: %s\n\n", fileToDelete.c_str());
                } else {
                    printf("Failed to delete file: %s\n", fileToDelete.c_str());
                }
                closedir(d);
                return;
            }
            printf("Fail to delete file [%s]: not found\n\n", fileToDelete.c_str());
            closedir(d);
        }
    }

        
    vector<string> handleLs(bool shouldPrint) {
        vector<string> dirs;
        struct FileMACTimes fileTimes[50]; 
        if (!receiveAll(socket, fileTimes, 50*sizeof(FileMACTimes))) {
            fprintf(stderr, "ERROR reading from socket\n");
            return dirs;
        }
        if (shouldPrint) cout << "file name - modified time - access time - created time";
        for (auto &directoryName: fileTimes) {
            if (!strcmp(directoryName.filename, "")) break;

            if (shouldPrint) cout << endl << directoryName.filename << " " << ctime(&directoryName.modifiedTime) << " " << ctime(&directoryName.accessedTime) << " " << ctime(&directoryName.createdTime);
            dirs.emplace_back(directoryName.filename);
        }
        
        return dirs;
    }

    void handleLc() {
        char location[256] = "sync_dir_";
        strcat(location, message.client);
        strcat(location, "/");

        DIR *d;
        struct dirent *dir;
        d = opendir(location);

        if (d) {
            cout << "file name - modified time - access time - created time";
            while ((dir = readdir(d))) {
                if (strcmp(dir->d_name, ".") != 0 && (strcmp(dir->d_name, "..") != 0)) {
                    struct stat fileStat;
                    string filePath = location;
                    filePath = filePath + "/" + dir->d_name;
                    if (stat(filePath.c_str(), &fileStat) == 0){
                        cout << endl << dir->d_name << " " << ctime(&fileStat.st_mtim.tv_sec) << " " << ctime(&fileStat.st_atim.tv_sec) << " " << ctime(&fileStat.st_ctim.tv_sec);
                    }
                }
            }
            closedir(d);
        }
        cout << endl;
    }

public:
    void handleGsd() {
        vector<string> dirs = handleLs(false);

        for (string fileName: dirs) {
            strcpy(message.content, "download ");
            strcat(message.content, fileName.c_str());
            splitCommand = splitString(message.content);
            if (!sendAll(socket, &message, sizeof(MESSAGE)))
                fprintf(stderr, "ERROR writing to socket\n");

            handleDownload(fileName);
        }
    }

    int handleInput() {
        // Remove \n
        message.content[strcspn(message.content, "\n")] = 0;

        splitCommand = splitString(message.content);
        const string &mainCommand = splitCommand[0];

        if (mainCommand == "upload") {
            handleUpload();
        } else if (mainCommand == "download") {
            handleDownload("");
        } else if (mainCommand == "delete") {
            handleDelete();
        } else if (mainCommand == "ls") {
            handleLs(true);
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