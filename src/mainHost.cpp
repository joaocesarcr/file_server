#include "host.cpp"

bool isHost = false;
int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "usage %s hostname\n", argv[0]);
        exit(-1);
    }
    if (argc < 5) {
        // É o host principal
        isHost = true;
    }

    if (isHost) {
        host();
    } else ring();
}