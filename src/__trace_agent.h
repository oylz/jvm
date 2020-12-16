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

bool st_ = false;
void sighandler(int signum){
    st_ = true; 
}

void JNICALL vm_init(jvmtiEnv *jvmti, JNIEnv *env, jthread thread){
    fprintf(stderr, "VMInit...\n");

    jvmtiError error = jvmti_->RunAgentThread(alloc_thread(env), &worker, NULL,
        JVMTI_THREAD_MAX_PRIORITY);
    CHECK_ERR
    
    signal(SIGUSR1, sighandler);
}

void stack_trace(){
        jvmtiStackInfo *stacks = NULL; 
        jint count = 0;
    
        jint rc = jvmti_->GetAllStackTraces(10, &stacks, &count); 
        fprintf(stderr, "\033[31m GetAllStackTraces, count:%d, rc:%d, tid:%x \033[0m\n",count, rc, get_tid());
        for(jint i = 0; i < count; i++){
            jvmtiStackInfo stack = stacks[i];
            jvmtiFrameInfo *info = stack.frame_buffer;  
            if(info == NULL){
                continue;
            }
            int fc = stack.frame_count;
            fprintf(stderr, "\033[31m\tstack%d\033[0m\n",i);
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

void JNICALL method_entry(jvmtiEnv* jvmti, 
    JNIEnv* jni, 
    jthread thread, 
    jmethodID method) throw(agent_exception){
    try{
        jvmtiError error;
        // class 
        jclass clazz;
        error = jvmti_->GetMethodDeclaringClass(method, &clazz);
        CHECK_ERR

        char* signature;
        error = jvmti_->GetClassSignature(clazz, &signature, 0);
        CHECK_ERR

        // method
        char* name;
        error = jvmti_->GetMethodName(method, &name, NULL, NULL);
        CHECK_ERR
       
        if(strncmp("DictBasePO", name, 10) == 0){
            fprintf(stderr, "\033[32m in method_entry:%s\033[0m\n", name);
        }
        else{
            fprintf(stdout, "in method_entry:%s\n", name);
        }

        // release
        error = jvmti_->Deallocate((unsigned char*)name);
        CHECK_ERR

        error = jvmti_->Deallocate((unsigned char*)signature);
        CHECK_ERR
    } 
    catch(const agent_exception& e){
        fprintf(stderr, "\033[31m method_entry error:[%s]\033[0m\n", e.what());
    }
}

void filter_class(char *signature, jlong size){
    if(strncmp("[Lcom/zoe", signature, 9)!=0 &&
        strncmp("Lcom/zoe", signature, 8)!=0){
        return;
    }
    fprintf(stderr, "\033[32m in vm_object_allloc, signature:%s, size:%lu\033[0m\n", 
        signature, size);
}
void JNICALL vm_object_alloc(jvmtiEnv *jvmti_env,
     JNIEnv* jni_env,
     jthread thread,
     jobject object,
     jclass object_klass,
     jlong size){
    char* signature;
    jvmtiError error = jvmti_->GetClassSignature(object_klass, &signature, 0);
    CHECK_ERR

    filter_class(signature, size);

    error = jvmti_->Deallocate((unsigned char*)signature);
    CHECK_ERR

    if(st_){
        stack_trace();
        st_ = false;
    }
}
void JNICALL class_file_load_hook(jvmtiEnv *jvmti_env,
     JNIEnv* jni_env,
     jclass class_being_redefined,
     jobject loader,
     const char* name,
     jobject protection_domain,
     jint class_data_len,
     const unsigned char* class_data,
     jint* new_class_data_len,
     unsigned char** new_class_data){
    if(name == NULL){
        return;
    }
    if(strncmp("[com/zoe", name, 8)!=0 &&
        strncmp("com/zoe", name, 7)!=0){
        return;
    }

    fprintf(stderr, "\033[33min class_file_load_hook, name:%s\033[0m\n", name);
}

static int           gc_count = 0;
static jrawMonitorID lock;
static void JNICALL worker(jvmtiEnv* jvmti, JNIEnv* jni, void *p){
    jvmtiError err;

    fprintf(stderr, "GC worker started...\n");

    for (;;) {
        err = jvmti_->RawMonitorEnter(lock);
        while (gc_count == 0) {
            err = jvmti_->RawMonitorWait(lock, 0); 
            if (err != JVMTI_ERROR_NONE) {
                err = jvmti_->RawMonitorExit(lock);
                return;
            }
        }
        gc_count = 0;
        

        /* Perform arbitrary JVMTI/JNI work here to do post-GC cleanup */
        fprintf(stderr, "post-GarbageCollectionFinish actions...\n");

        stack_trace();
        
        err = jvmti_->RawMonitorExit(lock);
    }   
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

    jvmtiError error = jvmti_->RawMonitorEnter(lock);
    CHECK_ERR
    gc_count++;
    error = jvmti_->RawMonitorNotify(lock);
    CHECK_ERR

    error = jvmti_->RawMonitorExit(lock);
    CHECK_ERR



    //fprintf(stderr, "begin sleep 30s\n");
    //usleep(30*1000*1000);
    //fprintf(stderr, "end sleep 30s\n");
}

void JNICALL gc_finish(jvmtiEnv *jvmti_env){
    fprintf(stderr, "\033[34muuuu end gc, tid:%x, tname:%s==============================================\033[0m\n", get_tid(), tname2());
}
void JNICALL exception_cb(jvmtiEnv *jvmti_env,
     JNIEnv* jni_env,
     jthread thread,
     jmethodID method,
     jlocation location,
     jobject exception){
    char* name;
    jvmtiError error = jvmti_->GetMethodName(method, &name, NULL, NULL);
    CHECK_ERR

    fprintf(stderr, "\033[35mnnnn exception_cb, name:%s\033[0m\n", name);

    error = jvmti_->Deallocate((unsigned char*)name);
    CHECK_ERR

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
        caps.can_generate_method_entry_events = 1;
        caps.can_generate_vm_object_alloc_events = 1;
        caps.can_generate_garbage_collection_events = 1;
        //caps.can_generate_exception_events = 1;
        jvmtiError error = jvmti_->AddCapabilities(&caps);
        CHECK_ERR
    } 
    void reg_event() const throw(agent_exception){
        jvmtiEventCallbacks callbacks;
        memset(&callbacks, 0, sizeof(callbacks));
        //callbacks.MethodEntry = method_entry;
        callbacks.VMInit = &vm_init;
        //callbacks.VMObjectAlloc = vm_object_alloc; 
        //callbacks.ClassFileLoadHook = class_file_load_hook;
        callbacks.GarbageCollectionStart = gc_start;
        callbacks.GarbageCollectionFinish = gc_finish;
        //callbacks.ExceptionCatch = exception_cb; 
        jvmtiError error;
        error = jvmti_->SetEventCallbacks(&callbacks, sizeof(callbacks));
        CHECK_ERR
    
        //error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, 0);
        //CHECK_ERR    

        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, 0);
        CHECK_ERR

        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, 0);
        CHECK_ERR    

        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, 0);
        CHECK_ERR    

        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, 0);
        CHECK_ERR 

        error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, 0);
        CHECK_ERR

        //error = jvmti_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION_CATCH, 0);
        //CHECK_ERR

        error = jvmti_->CreateRawMonitor("lock", &lock);
        CHECK_ERR
    }

};
#endif










