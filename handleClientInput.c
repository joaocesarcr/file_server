#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// TODO: Usar hash aqui ao invés de percorrer um array
//
// Function prototypes
void handleUpload();
void handleDownload();
void handleDelete();
void handleLs();
void handleLc();
void handleGsd();

// Command dictionary structure
typedef struct {
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
    {"exit", NULL} // Client side (client.c)
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

void handleLs() {
    printf("LS command selected.\n");
    // Your ls code here
}

void handleLc() {
    printf("LC command selected.\n");
    // Your lc code here
}

void handleGsd() {
    printf("GSD command selected.\n");
    // Your gsd code here
}

int handleInput(char* command) {
    char input[20];
    // Remove \n
    command[strcspn(command, "\n")] = 0;
    // Find the command in the dictionary
    int found = 0;
    for (int i = 0; i < numCommands; i++) {
        if (strcmp(command, commands[i].command) == 0) {
            if (commands[i].function != NULL) {
                commands[i].function();
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

