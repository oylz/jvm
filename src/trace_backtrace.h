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

// true: os::malloc
static bool trace_backtrace(std::vector<std::string> &stacks){
    unw_cursor_t cursor;
    unw_context_t context;
  
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);
    bool rr = false; 
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
        if(strncmp(name, "os::malloc", strlen("os::malloc")) == 0){
            rr = true;
        }
        stacks.push_back(std::string(name));
        if(name != symbol){
            free(name);
        }
    }
}


#endif




