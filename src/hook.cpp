#include "hook.h"
#include <sys/types.h>
#include <unistd.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include "mem_checker.h"

/* dlsym needs dynamic memory before we can resolve the real memory 
 * allocator routines. To support this, we offer simple mmap-based 
 * allocation during alloc_init_pending. 
 * We support a max. of ZALLOC_MAX allocations.
 * 
 * On the tested Ubuntu 16.04 with glibc-2.23, this happens only once.
 */
#define ZALLOC_MAX 1024
static void* zalloc_list[ZALLOC_MAX];
static size_t zalloc_cnt = 0;
void* zalloc_internal(size_t size)
{
  fprintf(stderr, "alloc.so: zalloc_internal called\n");
  if (zalloc_cnt >= ZALLOC_MAX-1) {
    fprintf(stderr, "alloc.so: Out of internal memory\n");
    return NULL;
  }
  /* Anonymous mapping ensures that pages are zero'd */
  void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
  if (MAP_FAILED == ptr) {
    fprintf(stderr, "alloc.so: zalloc_internal mmap failed\n");
    return NULL;
  }
  zalloc_list[zalloc_cnt++] = ptr; /* keep track for later calls to free */
  return ptr;
}


extern "C" {
typedef void *(*malloc_fun)(size_t size);
typedef void (*free_fun)(void *ptr);
typedef void *(*calloc_fun)(size_t nmemb, size_t size);
typedef void *(*realloc_fun)(void *ptr, size_t size);
typedef void *(*reallocarray_fun)(void *ptr, size_t nmemb, size_t size);

malloc_fun r_malloc = NULL;
free_fun r_free = NULL;
calloc_fun r_calloc = NULL;
realloc_fun r_realloc = NULL;
reallocarray_fun r_reallocarray = NULL;

static int alloc_init_pending = 1;

__attribute__((constructor))
static void trace_init(){
    fprintf(stderr, "\033[44m**beg*****trace_init\033[0m\n"); 
    alloc_init_pending = 1;
    r_malloc = (malloc_fun)dlsym(RTLD_NEXT, "malloc");
    r_free = (free_fun)dlsym(RTLD_NEXT, "free");
    r_calloc = (calloc_fun)dlsym(RTLD_NEXT, "calloc");
    r_realloc = (realloc_fun)dlsym(RTLD_NEXT, "realloc");
    r_reallocarray = (reallocarray_fun)dlsym(RTLD_NEXT, "reallocarray");
    mem_checker::start();
    alloc_init_pending = 0;
    fprintf(stderr, "\033[44m**end*****trace_init\033[0m\n"); 
}
__attribute__((destructor))
static void trace_destroy(){
    mem_checker::stop();
}

void *malloc(size_t size){
    if(alloc_init_pending){
        fprintf(stderr, "\033[44m**ee*****malloc\033[0m\n");
        return zalloc_internal(size);
    }
    if(mem_checker::pending_if_full(size))return NULL;
    return r_malloc(size);
}

void free(void *ptr){
    if(alloc_init_pending) {
        return;
    }
    for(size_t i = 0; i < zalloc_cnt; i++){
        if(zalloc_list[i] == ptr){
            /* If dlsym cleans up its dynamic memory allocated with zalloc_internal,
            * we intercept and ignore it, as well as the resulting mem leaks.
            * On the tested system, this did not happen
            */
            return;
        }
    }
    return r_free(ptr);
}

void *calloc(size_t nmemb, size_t size){
    if(alloc_init_pending) {
        fprintf(stderr, "\033[44m**ee*****calloc, %lx\033[0m\n", calloc);
        /* Be aware of integer overflow in nmemb*size.
        * Can only be triggered by dlsym */
        return zalloc_internal(nmemb*size);
    }
    if(mem_checker::pending_if_full(nmemb*size))return NULL;
    return r_calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size){
    if (alloc_init_pending) {
        fprintf(stderr, "alloc.so: realloc internal\n");
        if (ptr) {
            fprintf(stderr, "alloc.so: realloc resizing not supported\n");
            return NULL;
        }
        return zalloc_internal(size);
    }
    if(mem_checker::pending_if_full(size))return NULL;
    return r_realloc(ptr, size);
}

void *reallocarray(void *ptr, size_t nmemb, size_t size){
    fprintf(stderr, "\033[44m*******reallocarray\033[0m\n");
    return r_reallocarray(ptr, nmemb, size);
}

}


