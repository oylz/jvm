#!/bin/bash

#export JAVA_HOME=/usr/local/xyz/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/jdk


# ====beg jvm_inner====
Platform_os_family=linux
Platform_arch=x86
Platform_arch_model=x86_64
Platform_os_arch=linux_x86
Platform_os_arch_model=linux_x86_64
Platform_lib_arch=amd64
Platform_compiler=gcc


TARGET_DEFINES="-DTARGET_OS_FAMILY_${Platform_os_family} $TARGET_DEFINES"
TARGET_DEFINES="-DTARGET_ARCH_${Platform_arch} $TARGET_DEFINES"
TARGET_DEFINES="-DTARGET_ARCH_MODEL_${Platform_arch_model} $TARGET_DEFINES"
TARGET_DEFINES="-DTARGET_OS_ARCH_${Platform_os_arch} $TARGET_DEFINES"
TARGET_DEFINES="-DTARGET_OS_ARCH_MODEL_${Platform_os_arch_model} $TARGET_DEFINES"
TARGET_DEFINES="-DTARGET_COMPILER_${Platform_compiler} $TARGET_DEFINES"
TARGET_DEFINES="-DCOMPILER1 -DLINUX -D_GNU_SOURCE -DAMD64 $TARGET_DEFINES"

IINCLUDE="-I/usr/local/xyz/jdk-jdk8-b120/hotspot/src/cpu/x86/vm"
IINCLUDE="-I/usr/local/xyz/jdk-jdk8-b120/hotspot/src/os/posix/vm $IINCLUDE"
IINCLUDE="-I/usr/local/xyz/jdk-jdk8-b120/hotspot/src/os/linux/vm $IINCLUDE"
IINCLUDE="-I/usr/local/xyz/jdk-jdk8-b120/hotspot/src/os_cpu/linux_x86/vm $IINCLUDE"
IINCLUDE="-I/usr/local/xyz/jdk-jdk8-b120/hotspot/src/share/vm/precompiled $IINCLUDE"
IINCLUDE="-I/usr/local/xyz/jdk-jdk8-b120/hotspot/src/share/vm $IINCLUDE"
IINCLUDE="-I/usr/local/xyz/jdk-jdk8-b120/build/linux-x86_64-normal-server-release/hotspot/linux_amd64_compiler2/generated $IINCLUDE"

#JVM_INNER="-DJVM_INNER $TARGET_DEFINES $IINCLUDE"
# ====end jvm_inner====


function make_agent(){
    g++ -DOS_LINUX --std=c++11 --shared -fPIC -o libtrace_agent.so \
    -I$JAVA_HOME/include \
    -I$JAVA_HOME/include/linux \
    $JVM_INNER \
    src/main.cpp

    #-L/usr/local/xyz/jdk1.8.0_241/jre/lib/amd64/server -ljvm
    #-DMONITOR_EXCEPTION_AND_ALLOC \
}

function make_parser(){
    g++ --std=c++11 -o hsperfdata_parser src/main.cpp 
}

ts="src/ts/common/sysutil.cc src/ts/signal_handler.cc"
# google::Symbolize in glog
g++ -ggdb --std=c++11 --shared -fPIC -D_GNU_SOURCE -o libhook.so src/hook.cpp src/mem_checker.cpp $ts -ldl -lpthread -lunwind -lglog
make_agent

# test
#./hsperfdata_parser
#/home/xyz/code/jdk1.8.0_241/bin/jstat -gccapacity 5576
#sudo /home/xyz/code/jdk1.8.0_241/bin/jmap -heap 5576

