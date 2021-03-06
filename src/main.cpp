
#if 1
#include "trace_agent.h"

std::mutex agent_lock::mu_;
bool agent_lock::stop_ = false;
mem_fuller *mem_fuller::self_ = NULL;
thread_local bool mem_fuller::stack_trace_ = false;

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved){
    fprintf(stderr, "Agent_OnLoad\n");
    thoughtspot::StackTraceSignal::InstallInternalHandler();
    thoughtspot::StackTraceSignal::InstallExternalHandler();

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
    std::unique_lock<std::mutex> lock(agent_lock::mu_);
    agent_lock::stop_ = true;
}




#else

#include "hsperfdata_parser.h"
int main(int argc, char **argv){
    hsperfdata_parser::parse(argv[1]);
    return 0;
}
#endif


