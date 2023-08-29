#include "../include/host.hpp"
#include "../include/server.hpp"

vector<int> users_sockets;
vector<struct sockaddr_in> user_ips_in_host;
vector<int> user_ports_in_host;

int users_qtt = 0;
int new_ring_socket, new_hb_socket;
struct sockaddr_in cli_addr{};
socklen_t clilen;

[[noreturn]] void host(void *arg) {
    int PORT = (*(HostArgs *) arg).port;
    printf("%d", PORT);
    printf("Criando conexao\n");
    int sockfd = create_connection(PORT + 5);
    int socket_heartbeat = create_connection(PORT + 6);
    printf("Iniciando listen\n");
    listen(sockfd, 20);
    listen(socket_heartbeat, 20);

    while (true) {
        printf("waiting\n");
        new_ring_socket = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        new_hb_socket = accept(socket_heartbeat, (struct sockaddr *) &cli_addr, &clilen);
        printf("done\n");
        if (new_ring_socket == -1) {
            printf("ERROR on accept\n");
            continue;
        }
        printf("Active rings: %d\n", users_qtt + 1);

        users_sockets.push_back(new_ring_socket);
        user_ips_in_host.push_back(cli_addr);
        user_ports_in_host.push_back(PORT + users_qtt + 1);

        SERVER_MSG to_connect_to_first;
        to_connect_to_first.command = 1;
        to_connect_to_first.add = user_ips_in_host[0];
        to_connect_to_first.port = user_ports_in_host[0];

        SERVER_MSG to_connect_to_new;
        to_connect_to_new.command = 1;
        to_connect_to_new.port = PORT + users_qtt + 1;
        to_connect_to_new.add = cli_addr;

        SERVER_MSG useport;
        useport.port = PORT + users_qtt + 1;
        printf("%i", new_ring_socket);
        sendAll(new_ring_socket, &useport, sizeof(SERVER_MSG));
        printf("done\n");

        if (users_qtt == 0) {

        } else if (users_qtt == 1) {
            sendAll(users_sockets[0], &to_connect_to_new, sizeof(SERVER_MSG));
            sendAll(users_sockets[users_qtt], &to_connect_to_first, sizeof(SERVER_MSG));

        } else if (users_qtt > 1) {
            sendAll(users_sockets[users_qtt - 1], &to_connect_to_new, sizeof(SERVER_MSG));
            sendAll(new_ring_socket, &to_connect_to_first, sizeof(SERVER_MSG));
        }
        pthread_t th1, th2;
        pthread_create(&th1, nullptr, reinterpret_cast<void *(*)(void *)>(heart_beat_s), &new_hb_socket);

        auto *hostArgs = new HostArgs({PORT, nullptr});
        pthread_create(&th2, nullptr, reinterpret_cast<void *(*)(void *)>(server), hostArgs);
        users_qtt++;
    }
}

void heart_beat_s(void *arg) {
    HEART_BEAT message;
    int sockfd = *(int *) arg;
    bool running = true;
    while (running) {
        if (receiveAll(sockfd, &message, sizeof(HEART_BEAT)) <= 0)
            running = false;
        else {
            printf("HB \n");
        }
    }
}
