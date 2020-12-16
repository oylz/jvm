#include "hook.h"

void CollectedHeap::pre_full_gc_dump(GCTimer* timer){
    fprintf(stderr, "\033[31min pre_full_gc_dump\033[0m\n");
}

