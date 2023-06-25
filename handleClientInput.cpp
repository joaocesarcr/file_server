#include <cstdio>
#include <cstring>
#include <dirent.h>

#include "./h/message_struct.h"

// Function prototypes
void handleUpload();

void handleDownload();

void handleDelete(MESSAGE message);

void handleLs(MESSAGE message);

void handleLc();

void handleGsd();

int handleInput(MESSAGE message, int socket);

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
    printf("LS command selected!\n");
    // Your delete code here
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

    if (strcmp(message.command, "upload") == 0) {
        handleUpload();
    } else if (strcmp(message.command, "download") == 0) {
        handleDownload();
    } else if (strcmp(message.command, "delete") == 0) {
        handleDelete(message);
    } else if (strcmp(message.command, "ls") == 0) {
        handleLs(message);
    } else if (strcmp(message.command, "lc") == 0) {
        handleLc();
    } else if (strcmp(message.command, "gsd") == 0) {
        handleGsd();
    } else if (strcmp(message.command, "exit") == 0) {
        printf("Exiting the program.\n");
        return 0;
    } else {
        printf("Invalid command.\n");
    }

    return 0;
}

