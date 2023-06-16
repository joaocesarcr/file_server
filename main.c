#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./SerDes/message_struct.h"
#include "./SerDes/serializer.c"
#include "./SerDes/deserializer.c"

/*
 * TODO
 * Nao tem que enviar o tamanho, vai ser sempre fixo
 */


// Test the serialization and deserialization
int main() {
    // Create a struct instance
    MESSAGE original;
    original.number = 42;
    strncpy(original.data, "Deserialize", MAX_MESSAGE_LENGTH);
    original.data[MAX_MESSAGE_LENGTH] = '\0'; // Ensure null termination

    // Serialize the struct
    char* serializedData;
    serializeStruct(&original, &serializedData);

    // Send or store the serialized data
    // ...

    // Deserialize the struct
    MESSAGE deserialized;
    deserializeStruct(serializedData, &deserialized);

    // Print the deserialized struct
    printf("Number: %d\n", deserialized.number);
    printf("Message: %s\n", deserialized.data);

    // Free allocated memory
    free(serializedData);

    return 0;
}

