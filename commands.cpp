#include <dirent.h>
#include <cstdio>

int main() {
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != nullptr) {
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
    return (0);
}

