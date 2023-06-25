cmake_minimum_required(VERSION 3.25)
project(file_server)

set(CMAKE_CXX_STANDARD 14)

include_directories(h)

add_executable(file_server
        h/message_struct.hpp
        client.cpp
        commands.cpp
        handleClientInput.cpp
        server.cpp)
