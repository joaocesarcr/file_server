void deserializeStruct(const char* serializedData, MESSAGE* message);

void deserializeStruct(const char* serializedData, MESSAGE* message) {
    char* buffer = (char*)serializedData;

    // Copy the number
    memcpy(&(message->number), buffer, sizeof(int));
    buffer += sizeof(int);

    // Copy the message
    memcpy(message->data, buffer, MAX_MESSAGE_LENGTH);
    message->data[MAX_MESSAGE_LENGTH] = '\0';
}

