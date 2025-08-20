#ifndef RAND64_SW_H
#define RAND64_SW_H

#include <stdint.h>

void     sw_init (uint64_t seed);
uint64_t sw_next (void); 

#endif