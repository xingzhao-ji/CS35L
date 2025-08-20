#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include "options.h"

static void usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s [-i src] [-o out] BYTES\n"
        "  -i  src   rdrand | mrand48_r | /path/to/file (default: mrand48_r)\n"
        "  -o  out   stdio | N        (decimal block size, default: stdio)\n",
        prog);
}

bool parse_opts(int argc, char **argv, opts_t *o)
{
    // Initialize outpath to NULL
    *o = (opts_t){ .src = SRC_M48, .sink = SINK_STDIO, .block = 0, .outpath = NULL };

    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "i:o:")) != -1) {
        switch (c) {
        case 'i':
            if      (!strcmp(optarg, "rdrand"))      o->src = SRC_RDRAND;
            else if (!strcmp(optarg, "mrand48_r"))   o->src = SRC_M48;
            else {
                o->src = SRC_FILE;
                o->path = optarg;
            }
            break;
        case 'o':
            if (!strcmp(optarg, "stdio")) {
                o->sink = SINK_STDIO;
            } else {
                char *end = NULL;
                errno = 0;
                unsigned long n = strtoul(optarg, &end, 10);
                if (errno || *end != '\0') {
                    o->sink = SINK_FILE;
                    o->outpath = optarg;
                } else if (n == 0) {
                    usage(argv[0]);
                    return false;
                } else {
                    o->sink = SINK_WRITE;
                    o->block = n;
                }
            }
            break;
        case '?':                                       
            return false;
        default:
            usage(argv[0]);
            return false;
        }
    }

    if (optind != argc - 1) {
        usage(argv[0]);
        return false;
    }

    if (argv[optind][0] == '-') {            
        fprintf(stderr, "randall: BYTES must be positive decimal\n");
        return false;
    }

    char *end = NULL;
    errno = 0;
    o->total = strtoull(argv[optind], &end, 10);
    if (errno || *end) {
        fprintf(stderr, "randall: BYTES must be positive decimal\n");
        return false;
    }

    return true;
}