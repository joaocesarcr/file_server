cmake_minimum_required(VERSION 3.25)
project(file_server)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

add_executable(file_server
        include/message_struct.hpp
        src/client.cpp
        src/clientProcessor.cpp
        src/server.cpp
        src/serverProcessor.cpp
        src/utils.cpp)
