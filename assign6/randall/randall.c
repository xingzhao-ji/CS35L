#include <stdio.h>
#include "options.h"
#include "rand64-sw.h"
#include "rand64-hw.h"
#include "output.h"
#include "rand64-file.h"

static uint64_t rdrand_next_wrapper(void)
{
    uint64_t x;
    while (!hw_next(&x))         
        ;
    return x;
}

int main(int argc, char **argv)
{
    opts_t opt;
    if (!parse_opts(argc, argv, &opt)) {
        return 1;
    }

    uint64_t (*next64)(void) = sw_next; 
    sw_init(0);

    if (opt.src == SRC_RDRAND) {
        if (hw_available()) {
            next64 = rdrand_next_wrapper;
        }
    } else if (opt.src == SRC_M48) {
        sw_init(0);
        next64 = sw_next;
    } else if (opt.src == SRC_FILE) {
        if (file_init(opt.path) != 0) {
            perror(opt.path);
            return 1;
        }
        next64 = file_next64;
     }

    if (output_bytes(&opt, next64) != 0) {
        fputs("randall: I/O error\n", stderr);
        return 1;
    }
    return 0;
}