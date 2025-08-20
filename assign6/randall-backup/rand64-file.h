#ifndef RAND64_FILE_H
#define RAND64_FILE_H
#include <stdint.h>
int       file_init(const char *path);  
uint64_t  file_next64(void);            
void      file_fini(void);           
#endif
