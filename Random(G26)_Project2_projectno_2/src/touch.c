#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_WRONLY | O_CREAT | O_NOCTTY | O_NONBLOCK, 0666);
        if (fd == -1) {
            perror(argv[i]);
            continue;
        }
        
        if (utime(argv[i], NULL) == -1) {
            perror(argv[i]);
        }
        close(fd);
    }
    return 0;
}
