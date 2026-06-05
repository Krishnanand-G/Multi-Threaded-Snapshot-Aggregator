#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include "Message.h"

class BoundedQueue {
public:
    BoundedQueue(size_t capacity) : capacity_(capacity), active_(true) {}

    bool push(const TickMessage& msg) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_full_.wait(lock, [this]() { return queue_.size() < capacity_ || !active_; });
        if (!active_) return false;
        
        queue_.push(msg);
        lock.unlock();
        cond_empty_.notify_one();
        return true;
    }

    bool pop(TickMessage& msg) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_empty_.wait(lock, [this]() { return !queue_.empty() || !active_; });
        if (!active_ && queue_.empty()) return false;
        
        msg = queue_.front();
        queue_.pop();
        lock.unlock();
        cond_full_.notify_one();
        return true;
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        active_ = false;
        cond_empty_.notify_all();
        cond_full_.notify_all();
    }

private:
    std::queue<TickMessage> queue_;
    size_t capacity_;
    bool active_;
    std::mutex mutex_;
    std::condition_variable cond_empty_;
    std::condition_variable cond_full_;
};
