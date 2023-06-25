cmake_minimum_required(VERSION 3.25)
project(file_server C)

set(CMAKE_C_STANDARD 11)

include_directories(h)

add_executable(file_server
        h/message_struct.h
        client.c
        commands.c
        handleClientInput.c
        server.c)
