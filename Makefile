# Variables
CXX = g++
CXXFLAGS = --std=gnu++17 -pthread

# Executables
SERVER = server
CLIENT = client

# Source files
SERVER_SRCS = src/server.cpp src/utils.cpp
CLIENT_SRCS = src/client.cpp src/utils.cpp

# Default target
all: $(SERVER) $(CLIENT)

# Rule to compile the server
$(SERVER): $(SERVER_SRCS)
	$(CXX) $(CXXFLAGS) -o $(SERVER) $(SERVER_SRCS)

# Rule to compile the client
$(CLIENT): $(CLIENT_SRCS)
	$(CXX) $(CXXFLAGS) -o $(CLIENT) $(CLIENT_SRCS)

# Rule to run the server
run-server: $(SERVER)
	./$(SERVER) $(PORT)

# Rule to run the client
run-client: $(CLIENT)
	./$(CLIENT) $(USER) $(IP) $(PORT)

# Rule for cleaning
clean:
	rm -f $(SERVER) $(CLIENT)

.PHONY: all clean run-server run-client
