
## 关键点

* 获取jvm初始化参数(内存--mem_max低于2G报警--,环境变量)

* jvmti 关注 gc.start,gc.finish,thread.start,thread.end,exception 事件. 
    * gc.start 时调用 GetAllStackTraces (**需要注意可能会卡死**)---> 需要找一种机制在VMThread处理ParallelGCFailedAllocation前, 手动调下GetAllStackTraces
    * **需要关注 thread.end是否包含异常退出的线程事件**
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



```
java \
-Xms128m -Xmx128m \
-XX:+PrintGCTimeStamps -XX:+PrintGCDetails \
-XX:NewRatio=1 -XX:+UseCMSCompactAtFullCollection -XX:CMSFullGCsBeforeCompaction=2 \
-DDUBBO_GROUP=A \
-DNACOS_ADDR=172.16.34.114:8848 \
-DNACOS_GROUP=1dbe2a16-f6e4-49c6-99e6-d1780d5fa67d \
-agentpath:/home/xyz/zoenet/tool/tt/jvm/libtrace_agent.so \
-jar target/zoe-dubbo-consumer.jar
```


