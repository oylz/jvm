#ifndef _CHH_
#define _CHH_
#include <sys/types.h>

extern "C" {

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
}

#endif



