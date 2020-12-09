# some note

* https://blog.csdn.net/weixin_43194122/article/details/91526740

* java7后有两种办法来监听GC事件

    * 一种是比较传统的办法, 用C或C++或其它native语言来实现一个JVMTI agent,
        * 注册监听里面的 GarbageCollectionStart/GarbageCollectionFinish 事件即可.
        * 这两个事件都是在JVM处于GC **暂停阶段** 之中发出的, 此时不能执行任何Java代码.
        * 可以通过JVMTI的 GetStackTrace 函数来获取当时某个指定的Java线程的栈, 或者用 GetAllStackTraces 来获取所有Java线程的栈.
    
    * 另一种是用java7新推出的JMX API的GC notification
        * 用Java代码注册一个NotificationListener来监听GC事件即可. 这边的事件是在 **GC完成** 之后才发出的.
        * 用JMX版虽然可以用Java代码写, 挺方便, 但它获得GC事件通知时JVM已经不在暂停阶段, 所有Java线程都重新变成可运行的, 于是此时获取的stack trace就不如JVMTI准确.

* [Arthas运行原理](https://zhuanlan.zhihu.com/p/115127052)

* JVMTI（JVM Tool Interface）是Java虚拟机所提供的native接口，提供了可用于debug和profiler的能力，是实现调试器和其他运行态分析工具的基础，Instrument就是对它的封装。
* JVMTI又是在JPDA（Java Platform Debugger Architecture）之下的三层架构之一，JVMTI，JDWP，JDI。可以参考IBM系列文章：深入 Java 调试体系

===============================================================

# 0.GC日志

xx GC (reason) [PSYoungGen: young.before->young.after(young.cap)] [ParOldGen: old.before->old.after(old.cap)] heap.before->heap.after(heap.cap), [Metaspace: meta.before->meta.after(meta.cap)]

* Yong GC 
    * GC: 垃圾收集的类型是 Minor GC
    * Allocation Failure: 原因----young中没有足够的内存存放需要分配的数据
    * PSYoungGen: 垃圾收集器的名字
        * 424173K->5995K(477184K): gc前该区域使用量424m -> gc后该区域使用量5m(该区域容量47m)
    * 582834K->166947K(1001472K): gc前堆使用量582m -> gc后堆使用量166m(堆容量1G)

```
6429.226: [GC (Allocation Failure) [PSYoungGen: 424173K->5995K(477184K)] 582834K->166947K(1001472K), 0.0121254 secs] [Times: user=0.03 sys=0.00, real=0.01 secs]
79503.223: [GC (Allocation Failure) [PSYoungGen: 476960K->352K(503808K)] 648545K->171953K(1028096K), 0.0120187 secs] [Times: user=0.01 sys=0.00, real=0.02 secs]
```

* Full GC 
    * Full GC: 垃圾收集的类型是 Full GC
    * Ergonomics: 原因----全局GC
    * PSYoungGen:
        * 432128K->424896K(434176K): gc前该区域使用量432m -> gc后该区域使用量424m(该区域容量434m)
    * ParOldGen:
        * 524004K->524004K(524288K): gc前该区域使用量524m -> gc后该区域使用量524m(该区域容量524m)
    * 956132K->948901K(958464K): gc前堆使用量956m -> gc后堆使用量956m(堆容量1G)
    * Metaspace:

```
9131.949: [Full GC (Metadata GC Threshold) [PSYoungGen: 717K->0K(928768K)] [ParOldGen: 281354K->256922K(1048576K)] 282072K->256922K(1977344K), [Metaspace: 124488K->123404K(1165312K)], 0.5318096 secs] [Times: user=1.48 sys=0.00, real=0.53 secs] 

24768.074: [Full GC (Ergonomics) [PSYoungGen: 432128K->424896K(434176K)] [ParOldGen: 524004K->524004K(524288K)] 956132K->948901K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.8601840 secs] [Times: user=2.77 sys=0.00, real=0.86 secs] 
24769.732: [Full GC (Ergonomics) [PSYoungGen: 432128K->428851K(434176K)] [ParOldGen: 524004K->524004K(524288K)] 956132K->952855K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.6896198 secs] [Times: user=2.35 sys=0.00, real=0.69 secs] 
24768.943: [Full GC (Ergonomics) [PSYoungGen: 432128K->427890K(434176K)] [ParOldGen: 524004K->524004K(524288K)] 956132K->951895K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.7848190 secs] [Times: user=2.65 sys=0.00, real=0.78 secs] 
24770.426: [Full GC (Ergonomics) [PSYoungGen: 432128K->429299K(434176K)] [ParOldGen: 524004K->524004K(524288K)] 956132K->953303K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.7648835 secs] [Times: user=2.58 sys=0.00, real=0.77 secs] 
24771.194: [Full GC (Ergonomics) [PSYoungGen: 432128K->429725K(434176K)] [ParOldGen: 524004K->524004K(524288K)] 956132K->953730K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.7082118 secs] [Times: user=2.38 sys=0.00, real=0.71 secs] 
24771.913: [Full GC (Ergonomics) [PSYoungGen: 432128K->430364K(434176K)] [ParOldGen: 524004K->524003K(524288K)] 956132K->954367K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.8685206 secs] [Times: user=2.88 sys=0.00, real=0.87 secs] 
24772.790: [Full GC (Ergonomics) [PSYoungGen: 432128K->430625K(434176K)] [ParOldGen: 524003K->524003K(524288K)] 956131K->954629K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.7364953 secs] [Times: user=2.49 sys=0.00, real=0.73 secs] 
24773.529: [Full GC (Ergonomics) [PSYoungGen: 432128K->431229K(434176K)] [ParOldGen: 524003K->524003K(524288K)] 956131K->955233K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.7666966 secs] [Times: user=2.68 sys=0.00, real=0.76 secs] 
24774.298: [Full GC (Ergonomics) [PSYoungGen: 432128K->431281K(434176K)] [ParOldGen: 524003K->524003K(524288K)] 956131K->955284K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.7559722 secs] [Times: user=2.67 sys=0.00, real=0.76 secs] 
24775.055: [Full GC (Ergonomics) [PSYoungGen: 432128K->431337K(434176K)] [ParOldGen: 524003K->524003K(524288K)] 956131K->955341K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.6832452 secs] [Times: user=2.41 sys=0.00, real=0.68 secs] 
24775.740: [Full GC (Ergonomics) [PSYoungGen: 432128K->431391K(434176K)] [ParOldGen: 524003K->524003K(524288K)] 956131K->955395K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.8071272 secs] [Times: user=2.80 sys=0.00, real=0.80 secs] 
24776.550: [Full GC (Ergonomics) [PSYoungGen: 432128K->431382K(434176K)] [ParOldGen: 524003K->524003K(524288K)] 956131K->955386K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.7800767 secs] [Times: user=2.58 sys=0.00, real=0.78 secs] 
24777.333: [Full GC (Ergonomics) [PSYoungGen: 432128K->431386K(434176K)] [ParOldGen: 524003K->524003K(524288K)] 956131K->955389K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.8295454 secs] [Times: user=2.89 sys=0.00, real=0.83 secs] 
24778.165: [Full GC (Ergonomics) [PSYoungGen: 432128K->431374K(434176K)] [ParOldGen: 524003K->524003K(524288K)] 956131K->955377K(958464K), [Metaspace: 115712K->115712K(1163264K)], 0.8280003 secs] [Times: user=2.80 sys=0.00, real=0.83 secs] 
```

# 1.jmap

* java8之后没有 **永生代**, 原先永生代的信息会被放入 metaspace
* newratio(1, 表示 young与old比例是1:1)
* survivorratio(8, 表示young:2*survivor比例是8:2, 即1个survivor是young的1/10)
* young(PS Young Generation)
    * young.eden(Eden Space)
    * young.survivor1(From Space)
    * young.sruvivor2(To Space)
* old(PS Old Generation)


```
[root@base-hospital-5b996568d8-8r7sr /]# jmap -heap 1
Attaching to process ID 1, please wait...
Debugger attached successfully.
Server compiler detected.
JVM version is 25.45-b02

using thread-local object allocation.
Parallel GC with 4 thread(s)

Heap Configuration:
   MinHeapFreeRatio         = 0
   MaxHeapFreeRatio         = 100
   MaxHeapSize              = 1610612736 (1536.0MB)
   NewSize                  = 805306368 (768.0MB)
   MaxNewSize               = 805306368 (768.0MB)
   OldSize                  = 805306368 (768.0MB)
   NewRatio                 = 1 // ******* young:old 比例
   SurvivorRatio            = 8 // ******* young.eden:young.sruvivor 比例
   MetaspaceSize            = 134217728 (128.0MB)
   CompressedClassSpaceSize = 1073741824 (1024.0MB)
   MaxMetaspaceSize         = 268435456 (256.0MB) // *******
   G1HeapRegionSize         = 0 (0.0MB)

Heap Usage:
PS Young Generation // ******* 1.young(新生代)
Eden Space: // ******* 1.1.young.eden分布
   capacity = 747110400 (712.5MB)
   used     = 101852240 (97.13386535644531MB)
   free     = 645258160 (615.3661346435547MB)
   13.632823207922149% used
From Space: // ******* 1.2.young.survivor1分布
   capacity = 27787264 (26.5MB)
   used     = 6312760 (6.020317077636719MB)
   free     = 21474504 (20.47968292236328MB)
   22.718177651459317% used
To Space: // ******* 1.3.young.survivor2分布
   capacity = 25690112 (24.5MB)
   used     = 0 (0.0MB)
   free     = 25690112 (24.5MB)
   0.0% used
PS Old Generation // ******* 2.old(老年代)
   capacity = 805306368 (768.0MB)
   used     = 207018168 (197.42790985107422MB)
   free     = 598288200 (570.5720901489258MB)
   25.706759095191956% used


88156 interned Strings occupying 9182960 bytes.
```


# 2.arthas

```
Memory                                      used          total          max           usage          GC                                                                                            
heap                                        601M          1507M          1507M         39.95%         gc.ps_scavenge.count                               49                                         
ps_eden_space                               398M          712M           717M          55.58%         gc.ps_scavenge.time(ms)                            3919                                       
ps_survivor_space                           6M            26M            26M           22.72%         gc.ps_marksweep.count                              0                                          
ps_old_gen                                  197M          768M           768M          25.71%         gc.ps_marksweep.time(ms)                           0 
nonheap                                     187M          195M           1520M         12.34% 
code_cache                                  57M           58M            240M          24.07% 
metaspace                                   117M          123M           256M          45.88% 
compressed_class_space                      12M           13M            1024M         1.21% 
direct                                      1M            1M             -             100.00% 
mapped                                      0K            0K             -             0.00%  
```


