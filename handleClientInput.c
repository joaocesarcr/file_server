#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#include "./h/message_struct.h"

// TODO: Usar hash aqui ao invés de percorrer um array
//
// Function prototypes
void handleUpload();
void handleDownload();
void handleDelete();
void handleLs(MESSAGE message);
void handleLc();
void handleGsd();

// Command dictionary structure
typedef struct COMMAND_T {
    const char* command;
    void (*function)();
} COMMAND;

// Command dictionary
COMMAND commands[] = {
    {"upload", handleUpload},
    {"download", handleDownload},
    {"delete", handleDelete},
    {"ls", handleLs},
    {"lc", handleLc},
    {"gsd", handleGsd},
    {"exit", NULL} // Implemented on client.c and server.c
};

// Number of commands in the dictionary
const int numCommands = sizeof(commands) / sizeof(commands[0]);

// Function implementations
void handleUpload() {
  printf("Upload command selected.\n");
    // Your upload code here
}

void handleDownload() {
  printf("Download command selected.\n");
    // Your download code here
}

void handleDelete() {
  printf("Delete command selected.\n");
    // Your delete code here
}

void handleLs(MESSAGE data) {
    printf("LS command selected!\n");
    printf("Client name: %s\n", data.client);
    

    char location[256] = "server_files/"; // Declare 'location' as an array of characters
    strcat(location, data.client);
    printf("Location: %s\n", location);
    
    DIR *d;
    struct dirent *dir;
    d = opendir(location);
    
    if (d) {
        while ((dir = readdir(d)) != NULL) {
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

int handleInput(MESSAGE message) {
    // Remove \n
    message.command[strcspn(message.command, "\n")] = 0;
    // Find the command in the dictionary
    int found = 0;
    for (int i = 0; i < numCommands; i++) {
        if (strcmp(message.command, commands[i].command) == 0) {
            if (commands[i].function != NULL) {
                commands[i].function(message);
            } else {
                printf("Exiting the program.\n");
                return 0;
            }
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Invalid command.\n");
    }
    return 0;
}

