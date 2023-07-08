#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sstream>
#include <vector>


#include "./h/message_struct.hpp"

#define PORT 4000

using namespace std;

int createConnection(char *argv[]);

bool checkConnectionAcceptance(int socket);

std::vector<std::string> splitString(const std::string &str);
// ./myClient <username> <server_ip_address> <port>

int main(int argc, char *argv[]) {
    /*
      if (strcmp(argv[1], "content\n")) {
        printf("Testando inputs\n");
        printf("Enter a content: ");
        scanf("%s", input);

      }
    */
    if (argc < 4) {
        fprintf(stderr, "usage %s hostname\n", argv[0]);
        exit(-1);
    }

    int sockfd = createConnection(argv);
    char buffer[MAX_MESSAGE_LENGTH];

    MESSAGE message;
    strncpy(message.client, argv[1], MAX_MESSAGE_LENGTH);
    int running = 1;
    do {
        ssize_t n;
        //strncpy(message.content, "Sending packet", MAX_MESSAGE_LENGTH);
        //printf("teste: %s\n", message.data);
        std::string temp;
        printf("%s: ", message.client);
        getline(std::cin, temp);
        strcpy(message.content, temp.c_str());

        vector<string> splitCommand = splitString(message.content);
        const string &mainCommand = splitCommand[0];
        string secondArg;
        if (splitCommand.size() > 1)
            secondArg = splitCommand[1];
        /* write in the socket */
        n = write(sockfd, (void *) &message, sizeof(MESSAGE));
        if (n < 0)
            printf("ERROR writing to socket\n");
        if (mainCommand == "exit") {
            running = 0;
        } else if (mainCommand == "ls") {
            char directoryNames[50][256];
            do {
                n = read(sockfd, directoryNames, 12800);
            } while (n < sizeof(message));
            for (auto &directoryName: directoryNames) {
                if (!strcmp(directoryName, ""))
                    break;
                printf("%s", directoryName);
            }
        } else if (mainCommand == "download") {
            long size = 0;
            printf("Getting file size...\n");
            n = read(sockfd, (void *) &size, sizeof(long));
            printf("File size: %ld\n", size);

            FILE *file;
            char *buffer = new char[size + 1];
            // TODO: colocar o argumento
            file = fopen(secondArg.c_str(), "wb+");
            if (!file) {
                printf("Error opening file");
            }

            ssize_t bytesRead;

            n = read(sockfd, (void *) buffer, size);
            fwrite(buffer, size, 1, file);
            delete[] buffer;
            fclose(file);

        } else if (mainCommand == "upload") {
            FILE *file = fopen(secondArg.c_str(), "rb");
            if (file) {
                std::string fileName = secondArg.substr(secondArg.find_last_of("/") + 1);
                std::string serverFilePath = "server_files/" + std::string(message.client) + "/" + fileName;

                fseek(file, 0, SEEK_END);
                ssize_t fileSize = ftell(file);
                fseek(file, 0, SEEK_SET);

                n = write(sockfd, (void *) &fileSize, sizeof(ssize_t));
                if (n <= 0) {
                    printf("Error sending file size\n");
                    fclose(file);
                    continue;
                }

                const int BUFFER_SIZE = 1024;
                char buffer[BUFFER_SIZE];
                ssize_t bytesRead;
                ssize_t totalBytesSent = 0;

                while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                    n = write(sockfd, buffer, bytesRead);
                    if (n <= 0) {
                        printf("Error sending file data\n");
                        fclose(file);
                        break;
                    }

                    totalBytesSent += bytesRead;
                }

                printf("Total bytes sent: %zd\n", totalBytesSent);
                fclose(file);
            } else {
                printf("Error opening file\n");
            }
        }
        /*
         * TODO: vai precisar de uma classe pra saber lidar com a resposta
         * de cada comando possível do cliente (a resposta nem sempre é do
         * mesmo tamanho/tipo)
        else { 
          bzero(buffer, 256);

          do {
              n = read(sockfd, buffer, MAX_MESSAGE_LENGTH);
          } while (n < MAX_MESSAGE_LENGTH);
          printf("Answer: %s\n\n", buffer);
        }
        */

    } while (running);
    printf("Ending connection\n");

    close(sockfd);
    return 0;
}

int createConnection(char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr{};
    struct hostent *server;

    server = gethostbyname(argv[2]);
    if (!server) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(-1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        printf("ERROR opening socket\n");
        exit(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr = *((struct in_addr *) server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("ERROR establishing connection. Exiting...\n");
        exit(-1);
    }

    if (!checkConnectionAcceptance(sockfd)) exit(-1);

    printf("Connection established successfully.\n\n");
    return sockfd;

}

std::vector<std::string> splitString(const std::string &str) {
    string s;

    stringstream ss(str);

    vector<string> v;
    while (getline(ss, s, ' ')) {
        if (s != " ") {
            v.push_back(s);
        }
    }

    return v;
}

bool checkConnectionAcceptance(int socket) {
    size_t n;
    MESSAGE message;
    do {
        n = read(socket, (void *) &message, sizeof(message));
    } while (n < sizeof(message));

    if (strcmp(message.content, "accepted\0") == 0) return true;

    printf("ERROR: Connections quota reached\n");

    return false;
}

