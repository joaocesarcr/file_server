#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sstream>
#include <vector>
#include <string>

#include "./h/message_struct.hpp"

using namespace std;

// Function prototypes
void handleUpload();

void handleDownload();

void handleDelete(MESSAGE message);

void handleLs(MESSAGE message);

void handleLc();

void handleGsd();

int handleInput(MESSAGE message, int socket);

std::vector<std::string> splitString(const std::string &str) {
    string s;

    stringstream ss(str);

    vector<string> v;
    printf("");
    while (getline(ss, s, ' ')) {
        if (s != " ") {
            v.push_back(s);
        }
    }

    return v;
}

// Function implementations
void handleUpload() {
    printf("Upload command selected.\n");
    // Your upload code here
}

void handleDownload() {
    printf("Download command selected.\n");
    // Your download code here
}

void handleDelete(MESSAGE message) {
    printf("Delete command selected.\n");
    printf("Client name: %s\n", message.client);

    char location[256] = "server_files/"; // Declare 'location' as an array of characters
    strcat(location, message.client);
    printf("Location: %s\n", location);

    DIR *d;
    struct dirent *dir;
    d = opendir(location);

    if (d) {
        while ((dir = readdir(d)) != nullptr) {
            if (dir->d_name == message.splitCommand[1]) {
                strcat(location, "/");
                strcat(location, dir->d_name);
                remove(location);
                printf("Successfully deleted file: %s\n\n", dir->d_name);
                break;
            }
        }
        closedir(d);
    }
}

void handleLs(MESSAGE message) {
    printf("LS command selected!\n");
    printf("Client name: %s\n", message.client);

    char location[256] = "server_files/"; // Declare 'location' as an array of characters
    strcat(location, message.client);
    printf("Location: %s\n", location);

    DIR *d;
    struct dirent *dir;
    d = opendir(location);

    if (d) {
        while ((dir = readdir(d)) != nullptr) {
            printf("%s\n", dir->d_name);
        }
        printf("\n");
        closedir(d);
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

int handleInput(MESSAGE message, int socket) {
    // Remove \n
    message.command[strcspn(message.command, "\n")] = 0;

    message.splitCommand = splitString(message.command);
    string mainCommand = message.splitCommand[0];

    if (mainCommand == "upload") {
        handleUpload();
    } else if (mainCommand == "download") {
        handleDownload();
    } else if (mainCommand == "delete") {
        handleDelete(message);
    } else if (mainCommand == "ls") {
        handleLs(message);
    } else if (mainCommand == "lc") {
        handleLc();
    } else if (mainCommand == "gsd") {
        handleGsd();
    } else if (mainCommand == "exit") {
        printf("Exiting the program.\n");
        return 0;
    } else {
        printf("Invalid command.\n");
    }

    return 0;
}

