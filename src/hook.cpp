#include "hook.h"
#include <sys/types.h>
#include <unistd.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include "mem_checker.h"

extern "C" {


extern void *__libc_malloc(size_t size);
extern void __libc_free(void *p);

static thread_local bool in_mem = false;


void *trace_malloc(size_t size){
    void *p = __libc_malloc(size);
    mem_checker::add(p, size);
    return p;
}
void *malloc(size_t size){
    if (in_mem){
        fprintf(stderr, "__libc_malloc\n");
        return __libc_malloc(size);
    }
    fprintf(stderr, "malloc 1\n");
    in_mem = true;
    void *p = trace_malloc(size);
    in_mem = false;
    fprintf(stderr, "malloc 2\n");
    return p;
}




void free(void *ptr){
    if (!in_mem){
        fprintf(stderr, "__libc_free 1\n");
        mem_checker::del(ptr);
    }
    fprintf(stderr, "__libc_free 2\n");
    __libc_free(ptr);
}


}


