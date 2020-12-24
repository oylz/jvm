#ifndef _TRACE_BACKTRACEH_
#define _TRACE_BACKTRACEH_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include <cxxabi.h>

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <string.h>

// true:
static bool trace_backtrace_and_check_if_safepoint()
{
    unw_cursor_t cursor;
    unw_context_t context;
  
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);
  
    bool rr = false;
    int n=0;

    // check
    unw_cursor_t first = cursor;
    while ( unw_step(&cursor) ) {
        unw_word_t ip, sp, off;
    
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
    
        char symbol[256] = {"<unknown>"};
        char *name = symbol;

        if ( !unw_get_proc_name(&cursor, symbol, sizeof(symbol), &off) ) {
            int status;
            if ( (name = abi::__cxa_demangle(symbol, NULL, NULL, &status)) == 0 ){
                name = symbol;
            }
        }

        if(NULL!=strstr(name, "compiler_thread_loop") ||
            NULL!=strstr(name, "safepoint") ||
            NULL!=strstr(name, "VM_GetAllStackTraces") 
            ){
            rr = true;
        } 
        if(name != symbol){
            free(name);
        }
        if(rr){
            break;
        }
    }
    if(rr){
        return rr;
    }
    // print
    fprintf(stderr, "\033[31m |\033[32m |\033[33m |\033[34m |\033[35m"
        "nnnn----tid:%x(%d), %s:%d----\033[0m\n",
        get_tid(), get_tid(),
         __FILE__, __LINE__);
 
    cursor = first;
    while ( unw_step(&cursor) ) {
        unw_word_t ip, sp, off;
    
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
    
        char symbol[256] = {"<unknown>"};
        char *name = symbol;

        if ( !unw_get_proc_name(&cursor, symbol, sizeof(symbol), &off) ) {
            int status;
            if ( (name = abi::__cxa_demangle(symbol, NULL, NULL, &status)) == 0 ){
                name = symbol;
            }
        }
    
        fprintf(stderr, "\033[31m |\033[32m |\033[33m |\033[34m |\033[35m" 
            "\t#%-2d" PRIxPTR " %s + 0x%" PRIxPTR "--tid:%x(%d)\n",
            #if 1
            ++n,
            name,
            static_cast<uintptr_t>(off), get_tid(), get_tid());
            #else
            "\t#%-2d 0x%016" PRIxPTR " sp=0x%016" PRIxPTR " %s + 0x%" PRIxPTR "\n",
            ++n,
            static_cast<uintptr_t>(ip),
            static_cast<uintptr_t>(sp),
            name,
            static_cast<uintptr_t>(off));
            #endif
        if(name != symbol){
            free(name);
        }
    }
    fprintf(stderr, "\033[31m |\033[32m |\033[33m |\033[34m |\033[35m"
        "uuuu----tid:%x(%d), %s:%d----\033[0m\n",
        get_tid(), get_tid(),
         __FILE__, __LINE__);
    return rr;
}

static void xyz_backtrace(){
    fprintf(stderr, "\033[31m |\033[32m |\033[33m |\033[34m |\033[35m"
      "\033[0mnnnn----tid:%x(%d), %s:%d----\n",
      get_tid(), get_tid(),
       __FILE__, __LINE__);
    void *buffer[20] = { NULL };
    char **trace = NULL;
    int i = 0;

    int size = backtrace(buffer, 20);
    trace = backtrace_symbols(buffer, size);
    if (NULL == trace) {
        return;
    }
    for(i = 0; i < size; ++i) {
        fprintf(stderr, "\033[31m |\033[32m |\033[33m |\033[34m |\033[35m\tbuffer:%lx----%s"
            "\033[0m\n",
            buffer[i], trace[i]);
    }
    free(trace);
    fprintf(stderr, "\033[31m |\033[32m |\033[33m |\033[34m |\033[35m"
      "\033[0muuuu----tid:%x(%d), %s:%d----\n",
      get_tid(), get_tid(),
       __FILE__, __LINE__);
}


#endif




