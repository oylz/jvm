#ifndef _MEM_CHECKERH_
#define _MEM_CHECKERH_
#include <pthread.h>
#include <stdint.h>

static const uint64_t LEFT_SPACE = 30*1024*1024;
class mem_checker{
public:
    static void start();
    static void stop();
    static bool pending_if_full(uint64_t in);
private:
    static void *run(void *in);
    static pthread_t tid_;
    static int s_mutex_;
    static bool stop_;
    static int m_mutex_;
    static uint64_t mem_;
    static thread_local bool in_backtrace_;
};

#endif


