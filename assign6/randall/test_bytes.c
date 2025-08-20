#include <stdio.h>
#include <stdint.h>

int main() {
    uint64_t r = 0x0123456789ABCDEF;
    uint8_t bytes[8];
    
    // Method 1: Current approach
    for (int j = 0; j < 8; j++) {
        bytes[j] = (r >> (j * 8)) & 0xFF;
    }
    
    printf("Method 1 (current): ");
    for (int i = 0; i < 8; i++) {
        printf("%02X ", bytes[i]);
    }
    printf("\n");
    
    // Method 2: Big-endian
    bytes[0] = (r >> 56) & 0xFF;
    bytes[1] = (r >> 48) & 0xFF;
    bytes[2] = (r >> 40) & 0xFF;
    bytes[3] = (r >> 32) & 0xFF;
    bytes[4] = (r >> 24) & 0xFF;
    bytes[5] = (r >> 16) & 0xFF;
    bytes[6] = (r >> 8) & 0xFF;
    bytes[7] = r & 0xFF;
    
    printf("Method 2 (big-endian): ");
    for (int i = 0; i < 8; i++) {
        printf("%02X ", bytes[i]);
    }
    printf("\n");
    
    return 0;
}
