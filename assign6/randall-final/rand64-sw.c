#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "rand64-sw.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

static int urandom_fd = -1;
static uint8_t mix_buffer[16];
static int mix_pos = 16;

void sw_init(uint64_t seed)
{
    (void)seed; 
    urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd < 0) {
        perror("open /dev/urandom");
        exit(1);
    }
    mix_pos = 16; 
}

uint64_t sw_next(void)
{
    if (mix_pos >= 8) {
        if (read(urandom_fd, mix_buffer, 16) != 16) {
            perror("read /dev/urandom");
            exit(1);
        }
        mix_pos = 0;
    }
    
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result |= ((uint64_t)mix_buffer[mix_pos + i]) << (i * 8);
    }
    mix_pos += 8;
    
    return result;
}

void sw_fini(void)
{
    if (urandom_fd >= 0) {
        close(urandom_fd);
        urandom_fd = -1;
    }
}