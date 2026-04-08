#ifndef RAND64_HW_H
#define RAND64_HW_H

#include <stdint.h>

int  hw_available(void);
int  hw_next(uint64_t *out64);

#endif
