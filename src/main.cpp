#include "trace_agent.h"

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved){
    fprintf(stderr, "Agent_OnLoad\n");

    try{
        trace_agent* agent = new trace_agent();
        agent->init(vm);
        agent->add_cap();
        agent->reg_event();
    } catch (const std::exception& e){
        fprintf(stderr, "\033[31mexception:[%s]\033[0m\n", e.what());
        return JNI_ERR;
    }
    
    return JNI_OK;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm){
    fprintf(stderr, "Agent_OnUnload\n");
}

/*
int main(int argc, char **argv){

    return 0;
}
*/


