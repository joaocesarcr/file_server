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
#include <netinet/in.h>
#include <arpa/inet.h>

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


struct HostArgs {
    int port;
    char **argv;
};

typedef struct server_msg_s {
    struct sockaddr_in add{};
    int command{};
    int port{};
} SERVER_MSG;

typedef struct heart_beat_Struct {
    int ignore; // dps voltar pra string
} HEART_BEAT;

bool receiveAll(int socket, void *buffer, size_t length);

bool sendAll(int socket, const void *buffer, size_t length);

void monitorSyncDir(void *arg);

void syncChanges(void *arg);

void createSyncDir(const string &clientName);

int create_connection(int port);

#endif //FILE_SERVER_UTILS_HPP
