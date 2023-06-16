#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message_struct.h"
#include "serializer.c"


/*
 * TODO
 * Nao tem que enviar o tamanho, vai ser sempre fixo
 */

// Function to deserialize the struct
void deserializeStruct(const char* serializedData, const size_t serializedSize, MESSAGE* message) {
    char* buffer = (char*)serializedData;

    // Copy the number
    memcpy(&(message->number), buffer, sizeof(int));
    buffer += sizeof(int);

    // Copy the message
    memcpy(message->data, buffer, MAX_MESSAGE_LENGTH);
    message->data[MAX_MESSAGE_LENGTH] = '\0';
}

// Test the serialization and deserialization
int main() {
    // Create a struct instance
    MESSAGE original;
    original.number = 42;
    strncpy(original.data, "Deserialize", MAX_MESSAGE_LENGTH);
    original.data[MAX_MESSAGE_LENGTH] = '\0'; // Ensure null termination

    // Serialize the struct
    char* serializedData;
    size_t serializedSize;
    serializeStruct(&original, &serializedData, &serializedSize);

    // Send or store the serialized data
    // ...

    // Deserialize the struct
    MESSAGE deserialized;
    deserializeStruct(serializedData, serializedSize, &deserialized);

    // Print the deserialized struct
    printf("Number: %d\n", deserialized.number);
    printf("Message: %s\n", deserialized.data);

    // Free allocated memory
    free(serializedData);

    return 0;
}

