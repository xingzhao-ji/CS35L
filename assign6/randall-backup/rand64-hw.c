#include "rand64-hw.h"
#include <stdint.h>

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#include <immintrin.h>

static int rdrand_supported(void)
{
    unsigned eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);
    return (ecx >> 30) & 1;
}

int hw_available(void)
{
    return rdrand_supported();
}

int hw_next(uint64_t *out64)
{
    for (int i = 0; i < 16; i++)
        if (_rdrand64_step((unsigned long long *)out64))
            return 1;
    return 0;
}

#else
int hw_available(void)            { return 0; }
int hw_next(uint64_t *out64)      { (void)out64; return 0; }
#endif
