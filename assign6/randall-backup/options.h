#ifndef OPTIONS_H
#define OPTIONS_H
#include <stddef.h>
#include <stdbool.h>

typedef enum { SRC_RDRAND, SRC_M48, SRC_FILE } src_t;
typedef enum { SINK_STDIO, SINK_WRITE, SINK_FILE } sink_t;  

typedef struct {
    size_t  total;
    src_t   src;
    sink_t  sink;
    size_t  block;
    const char *path;
    const char *outpath;  
} opts_t;

bool parse_opts(int, char **, opts_t *);
#endif