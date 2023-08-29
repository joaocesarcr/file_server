#include "../include/host.hpp"
#include "../include/ring.hpp"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage %s hostname\n", argv[0]);
        exit(-1);
    }

    int port = stoi(argv[1]);
    if (argc == 3) {
        auto *hostArgs = new HostArgs({port, argv});
        ring(hostArgs);
    } else {
        auto *hostArgs = new HostArgs({port, nullptr});
        host(hostArgs);
    };
}