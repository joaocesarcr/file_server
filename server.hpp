#ifndef SERVER_HPP
#define SERVER_HPP

#include <pthread.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <cerrno>
#include <sys/types.h>
#include <climits>
#include <filesystem>
#include <iostream>

#include "./serverProcessor.cpp"

void *client_thread(void *arg);

int create_connection(int port);

bool checkClientAcceptance(int sockfd, MESSAGE message);

void removeClientConnectionsCount(int sockfd);

MESSAGE getClientName(int sockfd);

#endif 
