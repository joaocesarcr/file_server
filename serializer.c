#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void serializeStruct(const MESSAGE* message, char** serializedData, size_t* serializedSize);
void serializeStruct(const MESSAGE* message, char** serializedData, size_t* serializedSize) {

    printf("Serializing:\n string: %s\n number: %d\n", message->data, message->number);

    *serializedSize = sizeof(int) + MAX_MESSAGE_LENGTH;

    // Allocate memory for the serialized data
    *serializedData = (char*)malloc(*serializedSize);
    char* buffer = *serializedData;

    // Copy the number
    memcpy(buffer, &(message->number), sizeof(int));
    buffer += sizeof(int);

    // Copy the message
    memcpy(buffer, message->data, MAX_MESSAGE_LENGTH);
}

