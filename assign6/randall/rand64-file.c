#include "rand64-file.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

static int fd = -1;
static uint8_t buf[4096];
static size_t  fill = 0, pos = 0;

int file_init(const char *path)
{
    fd = open(path, O_RDONLY);
    return fd >= 0 ? 0 : -1;
}

static void refill(void)
{
    ssize_t n = read(fd, buf, sizeof buf);
    if (n <= 0) {
        if (lseek(fd, 0, SEEK_SET) == -1) {
            perror("lseek");
            exit(1);
        }
        n = read(fd, buf, sizeof buf);
        if (n <= 0) {
            perror("read");
            exit(1);
        }
    }
    fill = (size_t)n;
    pos = 0;
}

uint64_t file_next64(void)
{
    uint64_t x = 0;
    for (int b = 0; b < 8; ++b) {
        if (pos == fill) refill();
        x |= (uint64_t)buf[pos++] << (8 * b);
    }
    return x;
}

void file_fini(void) { if (fd != -1) close(fd); }