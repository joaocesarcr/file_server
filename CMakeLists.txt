cmake_minimum_required(VERSION 3.15)
project(file_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(include)

add_executable(server ./src/server.cpp ./src/utils.cpp)
target_link_libraries(server Threads::Threads)

add_executable(client ./src/client.cpp ./src/utils.cpp)
target_link_libraries(client Threads::Threads)
