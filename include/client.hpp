#ifndef CLIENT_HEADER_H
#define CLIENT_HEADER_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#include "../src/clientProcessor.cpp"

int createConnection(char *argv[], int port);

bool checkConnectionAcceptance(char clientName[], int socket);

void makeSyncDir(char clientName[], int socket);

#endif  // CLIENT_HEADER_H