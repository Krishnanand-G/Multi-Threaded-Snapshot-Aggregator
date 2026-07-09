#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include "Message.h"

class BoundedQueue {
public:
    BoundedQueue(size_t capacity) : capacity_(capacity), active_(true), head_(0), tail_(0) {
        queue_.resize(capacity_);
    }

    bool push(const TickMessage& msg) {
        while (active_.load(std::memory_order_acquire)) {
            while (lock_.test_and_set(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            if (tail_ - head_ < capacity_) {
                queue_[tail_ % capacity_] = msg;
                tail_++;
                lock_.clear(std::memory_order_release);
                return true;
            }
            lock_.clear(std::memory_order_release);
            std::this_thread::yield();
        }
        return false;
    }

    bool pop(TickMessage& msg) {
        while (active_.load(std::memory_order_acquire) || head_ < tail_) {
            while (lock_.test_and_set(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            if (head_ < tail_) {
                msg = queue_[head_ % capacity_];
                head_++;
                lock_.clear(std::memory_order_release);
                return true;
            }
            lock_.clear(std::memory_order_release);
            if (!active_.load(std::memory_order_acquire)) return false;
            std::this_thread::yield();
        }
        return false;
    }

    void shutdown() {
        active_.store(false, std::memory_order_release);
    }

private:
    std::vector<TickMessage> queue_;
    size_t capacity_;
    std::atomic<bool> active_;
    
    // Cache line padding to prevent false sharing
    alignas(64) std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
    alignas(64) size_t head_;
    alignas(64) size_t tail_;
};

