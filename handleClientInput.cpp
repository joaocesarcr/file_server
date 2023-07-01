#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sstream>
#include <vector>
#include <string>

#include <sys/stat.h>
#include "./h/message_struct.hpp"

using namespace std;

class CommandHandler {
public:
    int socket;
    MESSAGE message;

    void handleUpload() {
        printf("Upload command selected.\n");
        
    }

    void handleDownload() {
        printf("Download command selected.\n");
        char location[256] = "server_files/"; // Declare 'location' as an array of characters
        strcat(location, message.client);
        int n;
        strcat(location, "/");
        strcat(location, message.splitCommand[1].c_str());

        printf("Location: %s\n", location);
        struct stat st;
        stat(location, &st);
        long size = st.st_size;
        printf("size: %ld\n", size);
        n = write(socket, (void*) &size, sizeof(long));
 
    }

    void handleDelete() {
        char returnMessage[MAX_MESSAGE_LENGTH + 1];
        int n = 0;
        printf("Delete command selected.\n");
        printf("Client name: %s\n", message.client);

        char location[256] = "server_files/"; // Declare 'location' as an array of characters
        strcat(location, message.client);
        printf("Location: %s\n", location);

        DIR *d;
        struct dirent *dir;
        d = opendir(location);

        if (d) {
            while ((dir = readdir(d))) {
                if (dir->d_name == message.splitCommand[1]) {
                    strcat(location, "/");
                    strcat(location, dir->d_name);
                    remove(location);

                    printf("Successfully deleted file: %s\n\n", message.splitCommand[1].c_str());
                    snprintf(returnMessage, sizeof(returnMessage), "%s deleted\n", message.splitCommand[1].c_str());
                    n = write(socket, returnMessage, MAX_MESSAGE_LENGTH);
                    closedir(d);
                    return;
                }
            }
            printf("Fail to delete file [%s]: not found\n\n", message.splitCommand[1].c_str());
            closedir(d);
        }
        snprintf(returnMessage, sizeof(returnMessage), "Failed to delete %s \n", message.splitCommand[1].c_str());
        n = write(socket, returnMessage, MAX_MESSAGE_LENGTH);
    }

    void handleLs() {
        printf("LS command selected!\n");
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
                if (strcmp(dir->d_name, ".") && (strcmp(dir->d_name, ".."))) {
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
        printf("LC command selected.\n");
        // Your lc code here
    }

    void handleGsd() {
        printf("GSD command selected.\n");
        // Your gsd code here
    }

    int handleInput() {
        // Remove \n
        message.command[strcspn(message.command, "\n")] = 0;

        message.splitCommand = splitString(message.command);
        const string &mainCommand = message.splitCommand[0];

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
            printf("Invalid command.\n\n");
        }

        return 0;
    }

private:
    std::vector<std::string> splitString(const std::string &str) {
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

