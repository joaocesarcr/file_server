#define MAX_MESSAGE_LENGTH 256
typedef struct MESSAGE_t {
    int number;
    char data[MAX_MESSAGE_LENGTH+ 1]; // Including space for null terminator
} MESSAGE;


