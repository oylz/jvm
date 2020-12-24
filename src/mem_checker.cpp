#include "mem_checker.h"
#include <unistd.h>
#include <stdio.h>
#include "trace_util.h"
#include <stdlib.h>
#include "trace_backtrace.h"

pthread_t mem_checker::tid_ = 0;
int mem_checker::s_mutex_ = 0;
bool mem_checker::stop_ = false;
int mem_checker::m_mutex_ = 0;
uint64_t mem_checker::mem_ = 0;
mem_fuller *mem_fuller::self_ = NULL;
thread_local bool mem_fuller::stack_trace_ = false;
thread_local bool mem_checker::in_backtrace_ = false;


void mem_checker::stop(){
    {
        trace_lock lock(s_mutex_); 
        stop_ = true;
    }
    pthread_join(tid_, NULL);
    fprintf(stderr, "we are exit finish!\n");
}

bool mem_checker::pending_if_full(uint64_t in){
    if(in_backtrace_){
        return false;
    }
    uint64_t mem = 0;
    {
        trace_lock lock(m_mutex_);
        mem = mem_;
    }
    uint64_t max = mem*1024 + in + LEFT_SPACE;
    if(max >= 750*1024*1024){
        if(mem_fuller::get_stack_trace()){
            return false;
        }
        in_backtrace_ = true;
        bool tmp = trace_backtrace_and_check_if_safepoint();
        in_backtrace_ = false;
        if(tmp){
            return false;
        }
        fprintf(stderr, "\033[41mmem:%lu, in:%lu\033[0m\n", mem*1024, in);
        bool rr = mem_fuller::instance()->pending();
        return rr;
    }
    return false;
}
void *mem_checker::run(void *in){
    mem_fuller::set_stack_trace();
    while(1){
        bool stop = false;
        {
            trace_lock lock(s_mutex_);
            stop = stop_;
        }
        if(stop){
            break;
        }
        FILE *fl = fopen("/proc/self/statm","r");
        if(fl == NULL){
            return NULL;
        }
        uint32_t rss = 0;
        uint32_t res = 0;
        fscanf(fl,"%u%u", &rss,&res);
        fclose(fl); 
        uint64_t mem = res*4;
        {
            trace_lock lock(m_mutex_);
            mem_ = mem;
        }
        //fprintf(stderr, "mem:%lu\n", mem);
        usleep(5*200*1000);
    }
}
void mem_checker::start(){
    mem_fuller::instance();
    int rr = pthread_create(&tid_, NULL, run, NULL);
    if(rr == -1){
        fprintf(stderr, "\033[31merror to create thread\033[0m\n");
    }
} 



