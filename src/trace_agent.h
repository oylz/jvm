#ifndef _TRACE_AGENTH_
#define _TRACE_AGENTH_
#include <stdio.h>
#include <jvmti.h>
#include <string.h>
#include <exception>
#include <string>
#include <unistd.h>

#include <sys/syscall.h>
#include <execinfo.h>
#include <signal.h>
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
static jthread alloc_thread(JNIEnv *env){
    jclass thrClass = env->FindClass("java/lang/Thread");
    if ( thrClass == NULL ) {
        fprintf(stderr, "Cannot find Thread class\n");
    }

    jmethodID cid = env->GetMethodID(thrClass, "<init>", "()V");
    if ( cid == NULL ) {
        fprintf(stderr, "Cannot find Thread constructor method\n");
    }
    jthread res = env->NewObject(thrClass, cid);
    if ( res == NULL ) {
        fprintf(stderr, "Cannot create new Thread object\n");
    }
    return res;
}




JavaVM *vm_ = NULL;
jvmtiEnv *jvmti_ = NULL;


struct agent_exception : public std::exception{
public:
	agent_exception(int err){
        err_ = err;
	}
	agent_exception(const agent_exception& e){
        this->err_ = e.err_;
	}
	const char *what() const noexcept override { 
        return std::to_string(err_).c_str(); 
    }
 
private:
	int err_;
};

#define CHECK_ERR \
        if (error != JVMTI_ERROR_NONE) {\
            throw agent_exception(error);\
        }\

void stack_trace(){
        jvmtiStackInfo *stacks = NULL; 
        jint count = 0;
    
        jint rc = jvmti_->GetAllStackTraces(100, &stacks, &count); 
        fprintf(stderr, "\033[31m GetAllStackTraces, count:%d, rc:%d, tid:%x \033[0m\n",count, rc, get_tid());
        for(jint i = 0; i < count; i++){
            jvmtiStackInfo stack = stacks[i];
            jvmtiFrameInfo *info = stack.frame_buffer;  
            if(info == NULL){
                continue;
            }
            int fc = stack.frame_count;
            jvmtiThreadInfo ti;
            jvmti_->GetThreadInfo(stack.thread, &ti);
            fprintf(stderr, "\033[31m\tstack%d, tname:%s \033[0m\n",i, ti.name);
            for(int c = 0; c < fc; c++){
                jvmtiFrameInfo ii = info[c];
                jmethodID method = ii.method;
               
                jclass clazz;
                jvmtiError error = jvmti_->GetMethodDeclaringClass(method, &clazz);
                CHECK_ERR
    
                char* signature;
                error = jvmti_->GetClassSignature(clazz, &signature, 0);
                CHECK_ERR
    
     
                char* name;
                error = jvmti_->GetMethodName(method, &name, NULL, NULL);
                CHECK_ERR
                fprintf(stderr, "\t\t%s.%s\n", signature, name);
                error = jvmti_->Deallocate((unsigned char*)name);
                CHECK_ERR
                error = jvmti_->Deallocate((unsigned char*)signature);
                CHECK_ERR
            }
        }
}


static zqueue<uint64_t> _queue;

static void JNICALL worker(jvmtiEnv* jvmti, JNIEnv* jni, void *p){
    for (;;) {
        uint64_t tm = 0;
        _queue.pop(tm);
        stack_trace();
    }   
}

void sighandler(int signum){
    _queue.push(gtm());
    //stack_trace();
}


void JNICALL vm_init(jvmtiEnv *jvmti, JNIEnv *env, jthread thread){
    fprintf(stderr, "VMInit...\n");

    jvmtiError error = jvmti_->RunAgentThread(alloc_thread(env), &worker, NULL,
        JVMTI_THREAD_MAX_PRIORITY);
    CHECK_ERR
 
    signal(SIGUSR1, sighandler);
}



static const char *tname2(){
    jthread th;
    jvmtiError error = jvmti_->GetCurrentThread(&th);
    if (error != JVMTI_ERROR_NONE) {
        return "error tname_a";
    }
    jvmtiThreadInfo info;
    error = jvmti_->GetThreadInfo(th, &info);
    if (error != JVMTI_ERROR_NONE) {
        return "error tname_b";
    }
    return info.name;
}

void JNICALL gc_start(jvmtiEnv *jvmti_env){
    fprintf(stderr, "\033[34mnnnn beg gc, tid:%x, tname:%s==============================================\033[0m\n", get_tid(), tname2());

    kill(0, SIGUSR1);
    //fprintf(stderr, "begin sleep 30s\n");
    //usleep(30*1000*1000);
    //fprintf(stderr, "end sleep 30s\n");
}

void JNICALL gc_finish(jvmtiEnv *jvmti_env){
    fprintf(stderr, "\033[34muuuu end gc, tid:%x, tname:%s==============================================\033[0m\n", get_tid(), tname2());
}

class trace_agent{
private:

public:
    trace_agent(){
        jvmti_ = NULL;
    }
    void init(JavaVM *vm) throw(agent_exception){
        vm_ = vm;
        jvmtiEnv *jvmti = 0;
        jint ret = vm->GetEnv((void**)&jvmti, JVMTI_VERSION_1_0);
        if(ret != JNI_OK || jvmti == 0){
            throw agent_exception(-1);
        }
        jvmti_ = jvmti;
    }
    void add_cap() throw(agent_exception){
        jvmtiCapabilities caps;
        memset(&caps, 0, sizeof(caps));
        caps.can_generate_garbage_collection_events = 1;
        //caps.can_generate_exception_events = 1;
        jvmtiError error = jvmti_->AddCapabilities(&caps);
        CHECK_ERR
    } 
    void reg_event() const throw(agent_exception){
        jvmtiEventCallbacks callbacks;
        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.VMInit = &vm_init;
        callbacks.GarbageCollectionStart = gc_start;
        callbacks.GarbageCollectionFinish = gc_finish;
        jvmtiError error;
        error = jvmti_->SetEventCallbacks(&callbacks, sizeof(callbacks));
        CHECK_ERR
    

        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, 0);
        CHECK_ERR


        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, 0);
        CHECK_ERR 

        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, 0);
        CHECK_ERR
    }

};
#endif










