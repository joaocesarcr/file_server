#include <pthread.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "./handleClientInput.cpp"

#define PORT 4000

void *client_thread(void *arg);

int create_connection();

int main(int argc, char *argv[]) {
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr{};
    clilen = sizeof(struct sockaddr_in);

    sockfd = create_connection();
    // tells the socket that new connections shall be accepted
    listen(sockfd, 5);
    printf("Waiting accept\n");

    // get a new socket with a new incoming connection

    while (true) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd == -1)
            printf("ERROR on accept\n");
        else {
            printf("Connection established successfully.\n\n");
            pthread_t th1;
            pthread_create(&th1, nullptr, client_thread, &newsockfd);
        }
    }

    return 0;
}

void *client_thread(void *arg) {
    MESSAGE message;
    int newsockfd = *(int *) arg;
    int running = 1;
    ssize_t n;
    while (running) {
        /* read from the socket */
        do {
            n = read(newsockfd, (void *) &message, sizeof(message));
        } while (n < sizeof(message));

        if (n < 0) {
            printf("ERROR reading from socket\n");
            return (void *) -1;
        }
        if (!strcmp(message.command, "exit")) {
            printf("Ending Connection\n");
            running = 0;
        } else {
            handleInput(message);
            /* write in the socket */
            char messageReceived[MAX_MESSAGE_LENGTH + 1];
            snprintf(messageReceived, sizeof(messageReceived), "I got your message: %s", message.command);
//            printf("teste: %s", messageReceived);
            n = write(newsockfd, messageReceived, MAX_MESSAGE_LENGTH);
            if (n < 0)
                printf("ERROR writing to socket\n");
        }
    }
    close(newsockfd);
    printf("Connection ended\n");
}

int create_connection() {
    int sockfd, bindReturn;
    struct sockaddr_in serv_addr{};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("ERROR opening socket\n");
        exit(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr.sin_zero), 8);

    //  assigns the address specified by addr to the socket referred to
    // by the file descriptor sockfd
    bindReturn = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (bindReturn < 0) {
        printf("ERROR on binding\n");
        exit(-1);
    }
    return sockfd;

}
