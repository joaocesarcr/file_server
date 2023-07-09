#ifndef FILE_SERVER_UTILS_HPP
#define FILE_SERVER_UTILS_HPP

#include <cstdio>

bool receiveAll(int socket, void* buffer, size_t length);

bool sendAll(int socket, const void* buffer, size_t length);

#endif //FILE_SERVER_UTILS_HPP
