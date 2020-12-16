#ifndef _CHH_
#define _CHH_
#include <stdio.h>

class GCTimer;

class CollectedHeap{
public:
    void pre_full_gc_dump(GCTimer* timer);
};

#endif



