#ifndef FILE_SERVER_UTILS_HPP
#define FILE_SERVER_UTILS_HPP

#include <cstdlib>
#include <string>
#include <netdb.h>
#include <ctime>
#include <pthread.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <cerrno>
#include <sys/types.h>
#include <sys/inotify.h>
#include <climits>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>

#include "./h/message_struct.hpp"

using namespace std;

#define PORT 4000
#define MAX_EVENTS 1024 /*Max. number of events to process at one go*/
#define LEN_NAME 16 /*Assuming that the length of the filename won't exceed 16 bytes*/
#define EVENT_SIZE  ( sizeof (struct inotify_event) ) /*size of one event*/
#define BUF_LEN     ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )) /*buffer to store the data of events*/
#define MAX_TOTAL_CONNECTIONS 5
#define MAX_CONNECTIONS_PER_CLIENT 2

struct ThreadArgs {
    int socket;
    string message;
};

bool receiveAll(int socket, void *buffer, size_t length);

bool sendAll(int socket, const void *buffer, size_t length);

void *monitor_sync_dir_folder(void *arg);

void *listenSocket(void* arg);

void *inotify_thread(void *arg);

void *listener_thread(void *arg);

void createSyncDir(const string &clientName);

#endif //FILE_SERVER_UTILS_HPP
