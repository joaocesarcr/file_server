#include "../include/utils.hpp"
#include "ring.cpp"

vector<int> users;
vector<struct sockaddr_in> user_ips;

int users_qtt = 0;

int host() {
    socklen_t clilen = sizeof(struct sockaddr_in);
    int sockfd = create_connection("?????");
    struct sockaddr_in cli_addr{};

    // tells the socket that new connections shall be accepted
    listen(sockfd, 20);
    printf("Waiting accept\n");

    // get a new socket with a new incoming connection

    while (true) {
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd == -1) {
            printf("ERROR on accept\n");
            continue;
        }
        users[users_qtt] = newsockfd;
        user_ips[users_qtt] = cli_addr;

        SERVER_MSG to_accept_new;
        to_accept_new.command = 0;

        SERVER_MSG to_connect_to_first;
        to_connect_to_first.command = 1;
        to_connect_to_first.add = user_ips[0];

        SERVER_MSG to_connect_to_new;
        to_connect_to_new.command = 1;
        to_connect_to_new.add = cli_addr;

        // Caso especial: se só tem uma conexao?

        // tell users[0] to_accept_new
        sendAll(users[0], &to_accept_new, sizeof(SERVER_MSG));

        // tell users[-1] to connect_to newuser
        sendAll(users[users_qtt], &to_connect_to_new, sizeof(SERVER_MSG));

        // tell newuser to_accept_new
        sendAll(users[users_qtt], &to_accept_new, sizeof(SERVER_MSG));
        // tell newuser to connect to users[0]
        sendAll(newsockfd, &to_connect_to_first, sizeof(SERVER_MSG));

        // Cria thread pra heart_beat e comandos
        printf("Connection established successfully.\n\n");
        pthread_t th1;
        pthread_create(&th1, nullptr, reinterpret_cast<void *(*)(void *)>(heart_beat_s), &newsockfd);
        pthread_create(&th1, nullptr, talkto,newsockfd);
        users_qtt++;
        }
    return 0;
}
