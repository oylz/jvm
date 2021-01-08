#include "mm.h"
#include <sys/types.h>
#include <unistd.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <stdio.h>

#define PROT_READ   0x1     /* Page can be read.  */
#define PROT_WRITE  0x2     /* Page can be written.  */
#define PROT_EXEC   0x4     /* Page can be executed.  */
#define PROT_NONE   0x0     /* Page can not be accessed.  */
#define PROT_GROWSDOWN  0x01000000  /* Extend change to start of
                       growsdown vma (mprotect only).  */
#define PROT_GROWSUP    0x02000000  /* Extend change to start of
                       growsup vma (mprotect only).  */

#define MAP_ANONYMOUS 0x20
#define MAP_PRIVATE 0x02        /* Changes are private.  */
#define MAP_FAILED	((void *) -1)

extern "C" {
typedef void *(*mmap_fun)(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
typedef int (*munmap_fun)(void *addr, size_t length);

void *libc6 = NULL;
mmap_fun r_mmap = NULL;
munmap_fun r_munmap = NULL;

#define TRACE_INIT if(r_mmap==NULL) init_map();

static void init_map(){
    fprintf(stderr, "init_map\n");
    #if 0
    libc6 = dlopen("libc.so.6", RTLD_LAZY|RTLD_GLOBAL);
    if(!libc6){
        fprintf(stderr, "Aieee\n");
        //exit(1);
    }
    #endif
    r_mmap = (mmap_fun)dlsym(RTLD_NEXT, "mmap");
    r_munmap = (munmap_fun)dlsym(RTLD_NEXT, "munmap");
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset){
    fprintf(stderr, "\033[41m**ee*****mmap\033[0m\n");
    TRACE_INIT
    void *p = r_mmap(addr, length, prot, flags, fd, offset);
    return p;
}
int munmap(void *addr, size_t length){
    fprintf(stderr, "\033[41m**ee*****munmap\033[0m\n");
    TRACE_INIT
    return r_munmap(addr, length);
}


}


