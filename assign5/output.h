#ifndef OUTPUT_H
#define OUTPUT_H
#include <stdint.h>
#include "options.h"

typedef uint64_t (*rng64_t)(void);

/* return 0 on success, non-zero on write error */
int output_bytes(const opts_t *o, rng64_t next64);

#endif