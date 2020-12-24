#!/bin/bash

function make_agent(){
    g++ -DOS_LINUX --std=c++11 --shared -fPIC -o libtrace_agent.so \
    -I$JAVA_HOME/include \
    -I$JAVA_HOME/include/linux \
    src/main.cpp 


    #-DMONITOR_EXCEPTION_AND_ALLOC \
}

function make_parser(){
    g++ --std=c++11 -o hsperfdata_parser src/main.cpp 
}

g++ -ggdb --std=c++11 --shared -fPIC -D_GNU_SOURCE -o libhook.so src/hook.cpp src/mem_checker.cpp -ldl -lpthread -lunwind
make_agent

# test
#./hsperfdata_parser
#/home/xyz/code/jdk1.8.0_241/bin/jstat -gccapacity 5576
#sudo /home/xyz/code/jdk1.8.0_241/bin/jmap -heap 5576

