#pragma once

#ifndef FILE_SERVER_RING_HPP
#define FILE_SERVER_RING_HPP

#ifndef UTILS_HPP
#include "../include/utils.hpp"
#endif

typedef struct ring_msg_S {
    int command; // 0= send PID; 1= ELECTED
    int pid;
} RING_MSG;

void ring(void *arg);

void listen_to_server(void *arg);

void handleServerCommand(SERVER_MSG message);

int createConnectionRing(char argv[], int port);

int host_connectionRing(int port);

void ring_commands(void *arg);

void receive_connections_thread(void *arg);

int connec_to_ring(struct sockaddr_in, int port);

bool isInParticipants(int pid);

#endif //FILE_SERVER_RING_HPP
