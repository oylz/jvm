
# some notes about jvm

* [memory](./memory.md)

* [jvmti](./jvmti.md)



* have a look at:
    * HeapDumpBeforeFullGC
    ```
    2.352: [Heap Dump (before full gc): Dumping heap to java_pid8423.hprof ...
    ```

    * GenCollectFull, GenCollectFullConcurrent


    * stack generated by manually crash
    ```
    Stack: [0x00007f6db39fb000,0x00007f6db3afb000],  sp=0x00007f6db3af9500,  free space=1017k
    Native frames: (J=compiled Java code, j=interpreted, Vv=VM code, C=native code)
    V  [libjvm.so+0x56e381]  CollectedHeap::pre_full_gc_dump(GCTimer*)+0x2b
    V  [libjvm.so+0xad124d]  PSParallelCompact::invoke_no_policy(bool)+0x1b7
    V  [libjvm.so+0xad1083]  PSParallelCompact::invoke(bool)+0x147
    V  [libjvm.so+0xa898ef]  ParallelScavengeHeap::do_full_collection(bool)+0x31
    V  [libjvm.so+0x56d485]  CollectedHeap::collect_as_vm_thread(GCCause::Cause)+0xe9
    V  [libjvm.so+0xc195f6]  VM_CollectForMetadataAllocation::doit()+0x16e
    V  [libjvm.so+0xc44fbd]  VM_Operation::evaluate()+0x67
    V  [libjvm.so+0xc435a5]  VMThread::evaluate_operation(VM_Operation*)+0x5f
    V  [libjvm.so+0xc43b70]  VMThread::loop()+0x418
    V  [libjvm.so+0xc432bd]  VMThread::run()+0x109
    V  [libjvm.so+0xa60217]  java_start(Thread*)+0x1bb
    ```

    ```
    Stack: [0x00007fe8862f4000,0x00007fe8863f4000],  sp=0x00007fe8863f2830,  free space=1018k
    Native frames: (J=compiled Java code, j=interpreted, Vv=VM code, C=native code)
    V  [libjvm.so+0x8ea6ca]  JvmtiExport::post_garbage_collection_start()+0xa6
    V  [libjvm.so+0x8ec390]  JvmtiGCMarker::JvmtiGCMarker()+0x42
    V  [libjvm.so+0x5beaf7]  SvcGCMarker::SvcGCMarker(SvcGCMarker::reason_type)+0x39
    V  [libjvm.so+0xc1983f]  VM_ParallelGCFailedAllocation::doit()+0x1d
    V  [libjvm.so+0xc44fbd]  VM_Operation::evaluate()+0x67
    V  [libjvm.so+0xc435a5]  VMThread::evaluate_operation(VM_Operation*)+0x5f
    V  [libjvm.so+0xc43b70]  VMThread::loop()+0x418
    V  [libjvm.so+0xc432bd]  VMThread::run()+0x109
    V  [libjvm.so+0xa60217]  java_start(Thread*)+0x1bb
    ```

* backtrace1
    * ./hotspot/src/share/vm/precompiled/precompiled.hpp
    ```
    #include "utilities/defaultStream.hpp"
    static void backtrace1(){
        static char buf[2000];
    
        fdStream st(defaultStream::output_fd());
    
        frame fr = os::current_frame();
    
        // see if it's a valid frame
        if (fr.pc()) {
           st.print_cr("backtrace1 Native frames: (J=compiled Java code, j=interpreted, Vv=VM code, C=native code)");
    
    
           int count = 0;
           Thread* _thread = ThreadLocalStorage::get_thread_slow();
    
           while (count++ < StackPrintLimit) {
              fr.print_on_error(&st, buf, sizeof(buf));
              st.cr();
              // Compiled code may use EBP register on x86 so it looks like
              // non-walkable C frame. Use frame.sender() for java frames.
              if (_thread && _thread->is_Java_thread() && fr.is_java_frame()) {
                RegisterMap map((JavaThread*)_thread, false); // No update
                fr = fr.sender(&map);
                continue;
              }
              if (os::is_first_C_frame(&fr)) break;
              fr = os::get_sender_for_C_frame(&fr);
           }
    
           if (count > StackPrintLimit) {
              st.print_cr("backtrace1 ...<more frames>...");
           }
    
           st.cr();
        }   
    }
    ```

    * ./hotspot/src/share/vm/gc_interface/gcCause.cpp
    ```
    const char* GCCause::to_string(GCCause::Cause cause) {
        fprintf(stderr, "\033[32m nnnn in GCCause::to_string \033[0m\n");
        backtrace1();
        fprintf(stderr, "\033[32m uuuu in GCCause::to_string \033[0m\n");
    ```

    * ./hotspot/src/share/vm/gc_interface/collectedHeap.cpp
    ```
    void CollectedHeap::pre_full_gc_dump(GCTimer* timer) {
        fprintf(stderr, "\033[32m nnnn in pre_full_gc_dump \033[0m\n");
        backtrace1();
        fprintf(stderr, "\033[32m uuuu in pre_full_gc_dump \033[0m\n");
    ```

    * ThreadLocalStorage::get_thread_slow()->name()











