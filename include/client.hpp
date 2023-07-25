#ifndef CLIENT_HEADER_H
#define CLIENT_HEADER_H

#include "../src/clientProcessor.cpp"

int createConnection(char *argv[], int port);

bool checkConnectionAcceptance(char clientName[], int socket);

void makeSyncDir(char clientName[], int socket);

#endif  // CLIENT_HEADER_H