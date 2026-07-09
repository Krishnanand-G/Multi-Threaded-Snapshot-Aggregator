#pragma once
#include <vector>
#include <atomic>
#include "Message.h"

// Single-Producer Single-Consumer Lock-Free Queue
class BoundedQueue {
public:
    BoundedQueue(size_t capacity) : capacity_(capacity), active_(true) {
        queue_.resize(capacity_);
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    bool push(const TickMessage& msg) {
        if (!active_.load(std::memory_order_acquire)) return false;
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % capacity_;
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue full
        }
        
        queue_[current_tail] = msg;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(TickMessage& msg) {
        size_t current_head = head_.load(std::memory_order_relaxed);
        if (current_head == tail_.load(std::memory_order_acquire)) {
            if (!active_.load(std::memory_order_acquire)) return false;
            return false; // Queue empty
        }
        
        msg = queue_[current_head];
        head_.store((current_head + 1) % capacity_, std::memory_order_release);
        return true;
    }

    void shutdown() {
        active_.store(false, std::memory_order_release);
    }

private:
    std::vector<TickMessage> queue_;
    size_t capacity_;
    std::atomic<bool> active_;
    
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};


