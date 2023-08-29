#ifndef FILE_SERVER_HOST_HPP
#define FILE_SERVER_HOST_HPP

#ifndef UTILS_HPP
#include "../include/utils.hpp"
#endif

void heart_beat_s(void *arg);

[[noreturn]] void host(void *arg);

#endif //FILE_SERVER_HOST_HPP
