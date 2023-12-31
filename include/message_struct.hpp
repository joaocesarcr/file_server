#pragma once

#define MAX_MESSAGE_LENGTH 256

typedef struct MESSAGE_t {
    char content[MAX_MESSAGE_LENGTH + 1]; // Including space for null terminator
    char client[MAX_MESSAGE_LENGTH + 1]; // Including space for null terminator
} MESSAGE;
