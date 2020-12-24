
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


