#ifndef FILE_SERVER_UTILS_HPP
#define FILE_SERVER_UTILS_HPP

#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <iostream>
#include <map>
#include <netdb.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

#include "./message_struct.hpp"

using namespace std;

#define MAX_TOTAL_CONNECTIONS 5
#define MAX_CONNECTIONS_PER_CLIENT 2
#define MAX_FILENAME_LENGTH 256

struct ThreadArgs {
    int socket;
    string message;
};


struct FileMACTimes {
    char filename[MAX_FILENAME_LENGTH];
    time_t modifiedTime;
    time_t accessedTime;
    time_t createdTime;
};

typedef struct server_msg_s {
    struct sockaddr_in add{};
    int command{}; // dps voltar pra string
} SERVER_MSG;

typedef struct ring_msg_s {
    int pid;
    int command; // dps voltar pra string
} RING_MSG;

bool receiveAll(int socket, void *buffer, size_t length);

bool sendAll(int socket, const void *buffer, size_t length);

void monitorSyncDir(void *arg);

void syncChanges(void *arg);

void createSyncDir(const string &clientName);

#endif //FILE_SERVER_UTILS_HPP
