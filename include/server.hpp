#ifndef SERVER_HPP
#define SERVER_HPP

#include "../src/serverProcessor.cpp"

void client_thread(void *arg);

bool checkClientAcceptance(int sockfd, MESSAGE message);

void removeClientConnectionsCount(const string& clientName);

MESSAGE getClientName(int sockfd);

void server(void *arg);

#endif 
