#include "mem_checker.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include "trace_backtrace.h"
#include <map>
#include <sys/syscall.h>
#include "zqueue.h"

static pid_t get_tid() {
    pid_t tid = syscall(__NR_gettid);
    return tid;
}

struct trace_lock{
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



static int m_mutex_ = 0;
static uint64_t mem_ = 0;
static std::map<void *, int64_t> map_;

void mem_checker::add(void *p, uint64_t len){

    if(1){
        trace_lock lock(m_mutex_);
        mem_ += len;
        map_.insert(std::make_pair(p, len));
        std::vector<std::string> stacks;
        trace_backtrace(stacks);
        fprintf(stderr, "\033[31m |\033[32m |\033[33m |\033[34m |\033[35m"
            "nnnn----tid:%x(%d), current mem:%lu, %s:%d----\033[0m\n",
            get_tid(), get_tid(),
            mem_,
         __FILE__, __LINE__);
        for(int i = 0; i < stacks.size(); i++){
            const std::string &stack = stacks[i];
            fprintf(stderr, "\033[31m |\033[32m |\033[33m |\033[34m |\033[35m" 
                "\t#%-2d" PRIxPTR " %s " PRIxPTR "--tid:%x(%d)\n",
                i,
                stack.c_str(),
                get_tid(), get_tid());
        }
        fprintf(stderr, "\033[31m |\033[32m |\033[33m |\033[34m |\033[35m"
            "uuuu----tid:%x(%d), %s:%d----\033[0m\n",
            get_tid(), get_tid(),
            __FILE__, __LINE__);

    }

}

void mem_checker::del(void *p){
    if(p == NULL){
        return;
    }

    while(1){
        trace_lock lock(m_mutex_);
        std::map<void *, int64_t>::iterator it = map_.find(p);
        if(it == map_.end()){
            break;
        }
        mem_ -= it->second;
        map_.erase(it);
        break;
    }

}




