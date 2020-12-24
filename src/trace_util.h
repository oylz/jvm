#ifndef _TRACE_UTILH_
#define _TRACE_UTILH_
#include <sys/syscall.h>
#include "zqueue.h"

static pid_t get_tid() {
  static bool lacks_gettid = false;
  if (!lacks_gettid) {
    pid_t tid = syscall(__NR_gettid);
    if (tid != -1) {
      return tid;
    }   
    lacks_gettid = true;
  }

  return getpid(); 
}

class trace_lock{
private:
    int *mutex_;
public:
    explicit inline trace_lock(int mutex){
        mutex_ = &mutex;
 
        while(!__sync_bool_compare_and_swap(mutex_, 0, 1)){
            usleep(10*1000);//sleep 10ms
        }
    }
    inline ~trace_lock(){
        __sync_bool_compare_and_swap(mutex_, 1, 0);
    }
};

class mem_fuller{
private:
    pthread_mutex_t c_mutex_;
    pthread_cond_t cond_;
    static thread_local bool stack_trace_;

    static mem_fuller *self_;
    mem_fuller(){
        c_mutex_ = PTHREAD_MUTEX_INITIALIZER;
        cond_ = PTHREAD_COND_INITIALIZER;
    }
public:
    zqueue<uint64_t> queue_;

    static mem_fuller *instance(){
        if(self_ == NULL){
            self_ = new mem_fuller();
        }
        return self_;
    }
    bool pending(){
        queue_.push(gtm());
        if(!stack_trace_){
            char tname[512] = {0};
            pthread_t ptid = pthread_self(); 
            pthread_getname_np(ptid, tname, 512);
            fprintf(stderr, "\033[42mbeg pending, tid:%x(%d), tname:%s\033[0m\n", 
                get_tid(), get_tid(), tname);
            pthread_mutex_lock(&c_mutex_);
            pthread_cond_wait(&cond_, &c_mutex_); 
            pthread_mutex_unlock(&c_mutex_);
            fprintf(stderr, "\033[42mend pending, tid:%x(%d), tname:%s\033[0m\n", 
                get_tid(), get_tid(), tname);
            return false;
        }
        return false;
    }

    static void set_stack_trace(){
        stack_trace_ = true;
    }
    static bool get_stack_trace(){
        return stack_trace_;
    }
    void notify(bool all){
        pthread_mutex_lock(&c_mutex_);
        pthread_cond_signal(&cond_);
        if(all){
            pthread_cond_broadcast(&cond_);
            return;
        }
        pthread_mutex_unlock(&c_mutex_);
    }
};
#endif


