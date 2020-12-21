#ifndef __ZQUEUE_H__
#define __ZQUEUE_H__

#include <queue>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>
#include <chrono>

static uint64_t gtm() {
    struct timeval tm;
    gettimeofday(&tm, 0);
    uint64_t re = ((uint64_t)tm.tv_sec)*1000*1000 + tm.tv_usec;
    return re;
}

template <typename T>
class zqueue {
private:
    std::queue<T> que_;
    std::mutex mutex_;
    std::condition_variable cv_;

public:
    void push(const T &t) {
        std::unique_lock<std::mutex> lock(mutex_);
        que_.push(t);
        lock.unlock();
        cv_.notify_one();
    }

    void pop(T &t) {
        std::unique_lock<std::mutex> lock(mutex_);
        while(que_.empty()) {
            cv_.wait(lock);
        }    
        t = que_.front();
        que_.pop();
    }


    bool try_pop(T &t) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        while(que_.empty()) {
            if(std::cv_status::timeout == cv_.wait_for(lock, std::chrono::seconds(2))){
                return false;
            }
        }    
        t = que_.front();
        que_.pop();
        return true;
    }

    int count() {
        std::unique_lock<std::mutex> lock(mutex_);
        return que_.size();
    }

    void clear() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!que_.empty()) {
            que_.pop();
        }
    }
};


#endif
