#ifndef _TRACE_AGENTH_
#define _TRACE_AGENTH_
#include <stdio.h>
#include <jvmti.h>
#include <string.h>
#include <exception>
#include <string>
#include <unistd.h>

#include <execinfo.h>
#include <signal.h>
#include "zqueue.h"
#include "hsperfdata_parser.h"
#include <mutex>
#include "trace_util.h"
#include <thread>

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

struct agent_lock{
public:
    static std::mutex mu_;
    static bool stop_;
};

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


void stack_trace(bool print){
    if(0){
        char *user = getenv("USER");
        std::string path = "/tmp/hsperfdata_";
        path += std::string(user);
        path += "/";
        path += std::to_string(getpid());
        hsperfdata_parser::parse(path);
    }
        jvmtiStackInfo *stacks = NULL; 
        jint count = 0;
        fprintf(stderr, "\033[31mbeg GetAllStackTraces, tid:%x \033[0m\n", get_tid()); 
        jint rc = jvmti_->GetAllStackTraces(100, &stacks, &count); 
        fprintf(stderr, "\033[31mend GetAllStackTraces, count:%d, rc:%d, tid:%x \033[0m\n",count, rc, get_tid());
        if(!print){
            return;
        }
        for(jint i = 0; i < count; i++){
            jvmtiStackInfo stack = stacks[i];
            jvmtiFrameInfo *info = stack.frame_buffer;  
            jvmtiThreadInfo ti;
            jvmti_->GetThreadInfo(stack.thread, &ti);

            if(info == NULL){
                fprintf(stderr, "\033[41m\tstack%d, tname:%s \033[0m\n",i, ti.name);
                continue;
            }
            int fc = stack.frame_count;
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
    mem_fuller::set_stack_trace();
    for (;;) {
        uint64_t tm = 0;
        if(!_queue.try_pop(tm)){
            std::unique_lock<std::mutex> lock(agent_lock::mu_);
            if(agent_lock::stop_){
                break;
            }
            continue;
        }
        stack_trace(false);
    }   
}


// beg safepoint
static void safepoint_notify(zqueue<uint64_t> *qq){
    while(1){
        fprintf(stderr, "notify one\n");
        mem_fuller::instance()->notify(false);
        uint64_t tm = 0;
        if(!qq->try_pop(tm)){
            continue;
        }
        break;
    }
    fprintf(stderr, "notify finish!\n");
}
static void JNICALL safepoint_worker(jvmtiEnv* jvmti, JNIEnv* jni, void *p){
    mem_fuller::set_stack_trace();
    for (;;) {
        uint64_t tm = 0;
        if(!mem_fuller::instance()->queue_.try_pop(tm)){
            std::unique_lock<std::mutex> lock(agent_lock::mu_);
            if(agent_lock::stop_){
                break;
            }
            continue;
        }

        fprintf(stderr, "is full, we beg getting stacktrace...\n");
        zqueue<uint64_t> qq;
        std::thread th(safepoint_notify, &qq);
        stack_trace(true);
        qq.push(1);
        th.join();
        fprintf(stderr, "is full, we end getting stacktrace..., and we will exit\n");
        mem_fuller::instance()->notify(true);
        exit(123);
    }   
}
// end safepoint


void JNICALL vm_init(jvmtiEnv *jvmti, JNIEnv *env, jthread thread){
    fprintf(stderr, "VMInit...\n");
    if(jvmti_ == NULL){
        fprintf(stderr, "jvmti_ is null...\n");
        return;
    }
    jvmtiError error = jvmti_->RunAgentThread(alloc_thread(env), &worker, NULL,
        JVMTI_THREAD_MAX_PRIORITY);
    CHECK_ERR

    error = jvmti_->RunAgentThread(alloc_thread(env), &safepoint_worker, NULL,
        JVMTI_THREAD_MAX_PRIORITY);
    CHECK_ERR

}



void JNICALL gc_start(jvmtiEnv *jvmti_env){
    fprintf(stderr, "\033[34mnnnn beg gc, tid:%x==============================================\033[0m\n", get_tid());
    _queue.push(gtm());
}

void JNICALL gc_finish(jvmtiEnv *jvmti_env){
    fprintf(stderr, "\033[34muuuu end gc, tid:%x==============================================\033[0m\n", get_tid());
}

void JNICALL thread_end(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread){
    jvmtiThreadInfo ti;
    jvmti_->GetThreadInfo(thread, &ti);

    fprintf(stderr, "\033[33m*******thread_end, tname:%s*******\033[0m\n", ti.name);
}
void JNICALL thread_start(jvmtiEnv *jvmti_env,
     JNIEnv* jni_env,
     jthread thread){
    jvmtiThreadInfo ti;
    jvmti_->GetThreadInfo(thread, &ti);

    fprintf(stderr, "\033[33m*******thread_start, tname:%s*******\033[0m\n", ti.name);
}
#ifdef MONITOR_EXCEPTION_AND_ALLOC
void JNICALL exception_fun(jvmtiEnv *jvmti_env,
     JNIEnv* jni_env,
     jthread thread,
     jmethodID method,
     jlocation location,
     jobject exception,
     jmethodID catch_method,
     jlocation catch_location){
    // Ljava/lang/ClassNotFoundException
    // Ljava/lang/NoSuchMethodException
    // Ljava/io/FileNotFoundException
    // Ljava/lang/ArrayIndexOutOfBoundsException

    char* class_name;
    jclass exception_class = jni_env->GetObjectClass(exception);
    jvmti_env->GetClassSignature(exception_class, &class_name, NULL);
    fprintf(stderr, "Exception: %s\n", class_name);    
    return;
    // stacktrace
    jclass throwable_class = jni_env->FindClass("java/lang/Throwable");
    jmethodID print_method = jni_env->GetMethodID(throwable_class, "printStackTrace", "()V");
    jni_env->CallVoidMethod(exception, print_method);
}
void JNICALL object_alloc(jvmtiEnv *jvmti_env,
     JNIEnv* jni_env,
     jthread thread,
     jobject object,
     jclass object_klass,
     jlong size){
    char* class_name;
    jvmti_env->GetClassSignature(object_klass, &class_name, NULL);
    if(0==strncmp("Ljava", class_name, 5) || 0==strncmp("[Ljava", class_name, 6) ||
        0==strncmp("Lorg", class_name, 4) || 0==strncmp("[Lorg", class_name, 5) ||
        0==strncmp("Lsun", class_name, 4) || 0==strncmp("[Lsun", class_name, 5)){
        return;
    }
    if(strlen(class_name) == 2){
        return;
    }
    fprintf(stderr, "object_alloc: %s\n", class_name);    
}
#endif

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
        caps.can_tag_objects = 1;
        caps.can_signal_thread = 1;

        #ifdef MONITOR_EXCEPTION_AND_ALLOC
        caps.can_generate_exception_events = 1;
        caps.can_generate_vm_object_alloc_events = 1;
        #endif

        jvmtiError error = jvmti_->AddCapabilities(&caps);
        CHECK_ERR
    } 
    void reg_event() const throw(agent_exception){
        jvmtiEventCallbacks callbacks;
        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.VMInit = &vm_init;
        callbacks.GarbageCollectionStart = gc_start;
        callbacks.GarbageCollectionFinish = gc_finish;
        callbacks.ThreadEnd = thread_end;
        callbacks.ThreadStart = thread_start;
        #ifdef MONITOR_EXCEPTION_AND_ALLOC
        callbacks.Exception = exception_fun;
        //callbacks.VMObjectAlloc = object_alloc;
        #endif

        jvmtiError error;
        error = jvmti_->SetEventCallbacks(&callbacks, sizeof(callbacks));
        CHECK_ERR
    

        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, 0);
        CHECK_ERR


        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, 0);
        CHECK_ERR 

        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, 0);
        CHECK_ERR
        
        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_END, 0);
        CHECK_ERR
        
        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_START, 0);
        CHECK_ERR

        #ifdef MONITOR_EXCEPTION_AND_ALLOC
        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, 0);
        CHECK_ERR

        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, 0);
        CHECK_ERR
        #endif
    }

};
#endif










