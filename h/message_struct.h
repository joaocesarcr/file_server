#define MESSAGE_LENGTH 288
#define MESSAGE_DATA_LENGTH 256
#define MAX_MESSAGE_LENGTH 256
// 32 int
// 256 str 
typedef struct MESSAGE_t {
    int number;
    char data[MAX_MESSAGE_LENGTH+ 1]; // Including space for null terminator
} MESSAGE;

