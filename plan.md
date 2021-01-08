
## 关键点

* 获取jvm初始化参数(内存--mem_max低于2G报警--,环境变量)

* jvmti 关注 gc.start,gc.finish,thread.start,thread.end,exception 事件. 
    * gc.start 时调用 GetAllStackTraces (**需要注意可能会卡死**)---> 需要找一种机制在VMThread处理ParallelGCFailedAllocation前, 手动调下GetAllStackTraces
    * **需要关注 thread.end是否包含异常退出的线程事件**---> 包含
    * exception事件拦截所有异常(启动时有很多异常, 这些可能是包冲突所致)

* VM Thread繁忙时(jstack -F 都卡死): 只能使用 gdb, 此时, 如何知道 java 的 stacktraces ?, dump?
```
sudo gdb attach $ppid --batch -ex "thread apply all bt"
```

* thread相关
    * druid存活情况
    * druid连接数监控
    * redis存活情况
    * dubbo handler卡死监控
    * zk监控

* 大对象alloc监控: JVMTI_EVENT_VM_OBJECT_ALLOC?


## gc线程

* ./hotspot/src/share/vm/gc_implementation/parallelScavenge/gcTaskManager.cpp
* ./hotspot/src/share/vm/gc_implementation/parallelScavenge/gcTaskThread.cpp

* vm operrations: ./hotspot/src/share/vm/runtime/vm_operations.hpp
    * ParallelGCFailedAllocation
    
## jstat is read from /tmp/hsperfdata_${user}/${pid}

* 也可以
```
    import java.lang.management.ManagementFactory
    import sun.jvmstat.monitor.*;
    
    name = ManagementFactory.runtimeMXBean.name
    pid  = name[0..<name.indexOf('@')]
    vmId = new VmIdentifier(pid)
    vm   = MonitoredHost.getMonitoredHost(vmId).getMonitoredVm(vmId, 0)
    
    println 'Y count :' + vm.findByName('sun.gc.generation.0.spaces').longValue()
    println 'O count :' + vm.findByName('sun.gc.generation.1.spaces').longValue()
```

## 内存相关
* 堆外内存监控
* -XX:NativeMemoryTracking=summary ---> jcmd 13906 VM.native_memory summary scale=MB
* hook alloc ---> 申请内存时, 如果当前物理内存快到上限, 即时打出stack(java&c++)上传
    * 使用jemalloc优化内存?
    * 内存快达上限时阻塞并打印堆栈


## others

* **-XX:TraceJVMTI=all** 

```
java \
-Xms128m -Xmx128m \
-XX:+PrintGCTimeStamps -XX:+PrintGCDetails \
-XX:NewRatio=1 -XX:+UseCMSCompactAtFullCollection -XX:CMSFullGCsBeforeCompaction=2 \
-DDUBBO_GROUP=A \
-DNACOS_ADDR=172.16.34.114:8848 \
-DNACOS_GROUP=1dbe2a16-f6e4-49c6-99e6-d1780d5fa67d \
-XX:TraceJVMTI=all \
-agentpath:/home/xyz/zoenet/tool/tt/jvm/libtrace_agent.so \
-jar target/zoe-dubbo-consumer.jar
```

* hibernate监控

```
[HibernateTransactionManager.java:402] : Found thread-bound Session
```


* sql返回行数大于阈值时抛异常


* GCC 有个 C 语言扩展修饰符 __attribute__((constructor))，可以让由它修饰的函数在 main() 之前执行，若它出现在共享对象中时，那么一旦共享对象被系统加载，立即将执行 __attribute__((constructor)) 修饰的函数



* some notes

    * ./hotspot/src/share/vm/runtime/thread.cpp
    ```
     863 void Thread::print_on_error(outputStream* st, char* buf, int buflen) const {
     864   if      (is_VM_thread())                  st->print("VMThread");
     865   else if (is_Compiler_thread())            st->print("CompilerThread");
     866   else if (is_Java_thread())                st->print("JavaThread");
     867   else if (is_GC_task_thread())             st->print("GCTaskThread");
     868   else if (is_Watcher_thread())             st->print("WatcherThread");
     869   else if (is_ConcurrentGC_thread())        st->print("ConcurrentGCThread");
     870   else st->print("Thread");
    
    ```
    *
```
 99 "Signal Dispatcher" #4 daemon prio=9 os_prio=0 tid=0x00007f85d4182000 nid=0x34d3 runnable [0x0000000000000000]
100    java.lang.Thread.State: RUNNABLE


```

* async-peofiler
    * [1](https://blog.csdn.net/fenglibing/article/details/103534117)

    ```
    为了能够准确的生成Java和native代码的确切性能报告，常用的方法是接收perf_events生成的调用堆栈，并将它们与AsyncGetCallTrace生成的调用堆栈进行匹配。
    此外Async-profiler还提供了一种可以在AsyncGetCallTrace失败的某些情况下，恢复堆栈跟踪的解决方法。
    
    与将地址转换为Java方法名称的Java代理相比较，使用perf_eventst的方式具有以下优点：
    
    它适用于较旧的Java版本，因为它不需要-XX:+PreserveFramePointer，这个参数只在JDK 8u60和更高版本中可用；
    不需要引入-XX:+PreserveFramePointer，因为它可能导致较高的性能开销，在极少数情况下可能高达10%；
    不需要生成映射文件来将Java代码地址映射到方法名；
    使用解释器帧；
    不需要为了后续的进一步分析而需要生成perf.data文件；
    ```

    * [2](https://www.jdon.com/55066)
    ```
    通过使用HotSpot JVM提供的AsyncGetCallTrace API来分析Java代码路径，并使用Linux的perf_events来分析本机代码路径，可以避免安全点偏差问题。
    
    换句话说，分析器将Java代码和本机代码路径的调用堆栈进行匹配，以产生准确的结果。
    ```

* Change local symbol to global in ELF 
```
objcopy --globalize-symbol=SYMBOL_NAME INPUT_FILE OUTPUT_FILE
```

* soname
```
readelf libtest.so -d
```
* change soname
```
patchelf --set-soname libtest.so ./libtest.so
```

* 可重入malloc&free
```
不可重入的几种情况：

使用静态数据结构，比如getpwnam，getpwuid：如果信号发生时正在执行getpwnam，信号处理程序中执行getpwnam可能覆盖原来getpwnam获取的旧值

调用malloc或free：如果信号发生时正在malloc（修改堆上存储空间的链接表），信号处理程序又调用malloc，会破坏内核的数据结构
使用标准IO函数，因为好多标准IO的实现都使用全局数据结构，比如printf(文件偏移是全局的)
函数中调用longjmp或siglongjmp：信号发生时程序正在修改一个数据结构，处理程序返回到另外一处，导致数据被部分更新。
```

* gperftools

gperftools出来的值也很小(67.9m), 远远小于pmap/top输出的(507m)

```
export LD_PRELOAD=/usr/lib64/libtcmalloc.so
export HEAPPROFILE=/home/lg/temp
export HEAP_PROFILE_ALLOCATION_INTERVAL= 1073741824
```

* gdb dump memory

* /proc/${pid}/smaps

    * Rss=Shared_Clean+Shared_Dirty+Private_Clean+Private_Dirty

```
7fc5a6e6e000-7fc5a6f71000 rw-p 00000000 00:00 0 
Size:               1036 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                 184 kB
Pss:                 184 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:       184 kB
Referenced:          184 kB
Anonymous:           184 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:        0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:		0
VmFlags: rd wr mr mw me ac sd 
```

* 清理系统内存
``` 
echo 3 > /proc/sys/vm/drop_caches，会清理系统的cache。

Writing to this will cause the kernel to drop clean caches, dentries and inodes from memory, causing that memory to become free.

1-to free pagecache， 2-to free dentries and inodes， 3-to free pagecache, dentries and inodes。
```
* grep正则

```
grep -rn -E "[0-9]+:[0-9]+|Rss" nn.log
```

* /proc/$pid/fd

* tools:
    * DTrace
    * SystemTap
* memory dump

sudo gdb attach 4479 --batch -ex "dump memory /home/xyz/zoenet/tool/tt/jvm/mm.bin 0x00007fc5a0000000 0x00007fc5a2d51000"
```
 879 00007fc5a0000000   46404   40920   40920 rw---   [ anon ]    
 880 00007fc5a0000000       0       0       0 rw---   [ anon ]                    
 881 00007fc5a2d51000   19132       0       0 -----   [ anon ]                    
 882 00007fc5a2d51000       0       0       0 -----   [ anon ]  
```

* 因为使用gperftools没有追踪到这些内存，于是直接使用命令strace -f -e"brk,mmap,munmap" -p pid追踪向OS申请内存请求，但是并没有发现有可疑内存申请。

* 因为使用strace没有追踪到可疑内存申请；于是想着看看内存中的情况。就是直接使用命令gdp -pid pid进入GDB之后，然后使用命令dump memory mem.bin startAddress endAddressdump内存，其中startAddress和endAddress可以从/proc/pid/smaps中查找。然后使用strings mem.bin查看dump的内容


* https://www.jianshu.com/p/38a4bcf564d5
```
简单来说，文章开头内存不断增长的趋势的根本原因是：glibc在利用操作系统的内存构建进程自身的内存池。由于进程本身处理请求量大，频繁调用new和delete，在一段时间内，进程不断的从操作系统获取内存来满足新增的调用要求，但是从最终结果上来讲，总有一个临界点，使得进程从操作系统新获取的内存和归还给操作系统的内存达成相对平衡。在这个动态平衡建立前，内存会不断增长，直到到达临界点。
按照这个理论，机器内存应该先涨后平。我们看下几天后，机器的内存趋势图
```

* https://blog.csdn.net/wscdylzjy/article/details/44244413
解决办法：
1. 使用内存池，程序改动较大，但对程序性能和管理都是很有益
2. 显示调用 malloc_trim(0) 来强制回收被释放的堆内存。
3. 调小M_TRIM_THRESHOLD （）

* gdb ---> call malloc_stats()

* 小结:
    * gperftools&libhook都显示实际使用内存很少, 说明程序没有泄漏
    * 那为什么top&pmap显示内存很多: 可能glic有问题
    * 那为什么org有多少内存吃多少: 可能碎片太多, 再申请时直接被oom了
    * 但是, 线上实际执行malloc_trim(0)后没有降低多少



* ../jdk-jdk8-b120/hotspot/src/share/vm/runtime/os.cpp
* ./hotspot/make/linux/makefiles/product.make
./hotspot/src/share/vm/gc_implementation/parallelScavenge/psOldGen.hpp


* 2021.01.03晚总结:
    * java启动时调用了相当部分的mmap占用内存, 猜测主要为heap. /home/xyz/code/jdk-jdk8-b120/hotspot/src/os/linux/vm/os_linux.cpp
    * java也有调用malloc, 但最终占用内存量很小: 81m. ./hotspot/src/share/vm/runtime/os.cpp

* ftrace:
    * https://zhuanlan.zhihu.com/p/33267453


## FINAL

* strace

生成内存申请的系统调用


```
sudo strace -T -tt -f -e "brk,mmap,munmap" -k -fp $ppid
```

* perf

可生成缺页中断时的线程栈信息, 可与strace同时使用

    * 采集
    ```
    sudo perf record -e page-faults -a -p $ppid -g
    sudo perf script > perf.stacks
    ```

    * 统计缺页中断产生的内存总量
    ```
    tt=0; for mm in `grep "page-faults" ./perf.stacks | awk '{print $4}'`;do let tt=$tt+$mm;done;echo "4096*$tt" | bc
    ```

    * perf.stacks输出样例
    ```
    java  8467  7394.315225:       1254 page-faults: 
    	    7f4c2db7ea5f __memmove_avx_unaligned_erms+0x4f (/lib/x86_64-linux-gnu/libc-2.27.so)
    	    7f4c2cfbd6c8 PSPromotionManager::drain_stacks_depth+0x6d8 (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2cfc086a PSEvacuateFollowersClosure::do_void+0x1a (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2cfc787c ReferenceProcessor::process_phase3+0xac (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2cfc8415 ReferenceProcessor::process_discovered_reflist+0x135 (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2cfc8985 ReferenceProcessor::process_discovered_references+0x165 (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2cfbf967 PSScavenge::invoke_no_policy+0x9a7 (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2cfc04d8 PSScavenge::invoke+0x38 (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2cf78803 ParallelScavengeHeap::failed_mem_allocate+0x63 (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2d0e4194 VM_ParallelGCFailedAllocation::doit+0x84 (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2d0e88c7 VM_Operation::evaluate+0x47 (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2d0e7268 VMThread::evaluate_operation+0x2a8 (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2d0e76c9 VMThread::loop+0x219 (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2d0e7b12 VMThread::run+0x72 (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2cf54fe2 java_start+0xf2 (/home/xyz/code/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk/lib/amd64/server/libjvm.so)
    	    7f4c2d5d46db start_thread+0xdb (/lib/x86_64-linux-gnu/libpthread-2.27.so)
    ```









