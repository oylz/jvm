#ifndef _MEM_CHECKERH_
#define _MEM_CHECKERH_
#include <pthread.h>
#include <stdint.h>

struct mem_checker{
public:
    static void add(void *p, uint64_t len);
    static void del(void *p);
};

#endif


