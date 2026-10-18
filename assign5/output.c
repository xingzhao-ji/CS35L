#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "output.h"

int output_bytes(const opts_t *o, rng64_t next64)
{
    FILE *outfile = stdout;
    int fd = STDOUT_FILENO;
    
    if (o->sink == SINK_FILE && o->outpath) {
        outfile = fopen(o->outpath, "wb");
        if (!outfile) {
            perror(o->outpath);
            return 1;
        }
        fd = fileno(outfile);
    }
    
    size_t blk = (o->sink == SINK_WRITE && o->block) ? o->block : 8192;
    uint8_t *buf = malloc(blk);
    if (!buf) {
        if (outfile != stdout) fclose(outfile);
        return 1;
    }

    size_t left = o->total;
    while (left) {
        size_t n = left < blk ? left : blk;
        
        for (size_t i = 0; i < n; ) {
            uint64_t r = next64();
            for (int j = 0; j < 8 && i < n; j++, i++) {
                buf[i] = (r >> (j * 8)) & 0xFF;
            }
        }
        
        if (o->sink == SINK_WRITE) {
            size_t done = write(fd, buf, n);
            if (done != n) {
                free(buf);
                if (outfile != stdout) fclose(outfile);
                return 1;
            }
        } else {
            if (fwrite(buf, 1, n, outfile) != n) {
                free(buf);
                if (outfile != stdout) fclose(outfile);
                return 1;
            }
        }
        
        left -= n;
    }
    
    free(buf);
    if (outfile != stdout) {
        return fclose(outfile) == 0 ? 0 : 1;
    }
    
    return fflush(outfile) == 0 ? 0 : 1;
}