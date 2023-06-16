void serializeStruct(const MESSAGE* message, char** serializedData);

void serializeStruct(const MESSAGE* message, char** serializedData) {

    printf("Serializing:\n  number: %d\n string: %s\n",message->number, message->data);

    // Allocate memory for the serialized data
    *serializedData = (char*)malloc(MESSAGE_LENGTH);
    char* buffer = *serializedData;

    // Copy the number
    memcpy(buffer, &(message->number), sizeof(int));
    buffer += sizeof(int);

    // Copy the message
    memcpy(buffer, message->data, MAX_MESSAGE_LENGTH);
}

