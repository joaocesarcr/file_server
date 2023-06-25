#define MESSAGE_LENGTH 288
#define MESSAGE_DATA_LENGTH 256
#define MAX_MESSAGE_LENGTH 256

#pragma once

typedef struct MESSAGE_t {
    char command[MAX_MESSAGE_LENGTH + 1]; // Including space for null terminator
    char client[MAX_MESSAGE_LENGTH + 1]; // Including space for null terminator
} MESSAGE;


