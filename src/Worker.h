#pragma once
#include <unordered_map>
#include <atomic>
#include <thread>
#include "Queue.h"
#include "ThreadCompat.h"

class SnapshotStore {
public:
    SnapshotStore() : sequence_(0) {}

    void update(const TickMessage& msg) {
        // Writer: Increment sequence to odd before writing
        sequence_.fetch_add(1, std::memory_order_release);
        std::atomic_thread_fence(std::memory_order_release);

        snapshots_[msg.symbol] = msg;

        // Writer: Increment sequence to even after writing
        std::atomic_thread_fence(std::memory_order_release);
        sequence_.fetch_add(1, std::memory_order_release);
    }

    std::unordered_map<std::string, TickMessage> getSnapshots() {
        std::unordered_map<std::string, TickMessage> copy;
        size_t seq1, seq2;
        do {
            // Reader: wait if sequence is odd (writer is active)
            do {
                seq1 = sequence_.load(std::memory_order_acquire);
                if (seq1 & 1) compat::this_thread::yield();
            } while (seq1 & 1);
            
            std::atomic_thread_fence(std::memory_order_acquire);
            copy = snapshots_;
            std::atomic_thread_fence(std::memory_order_acquire);
            
            seq2 = sequence_.load(std::memory_order_acquire);
        } while (seq1 != seq2); // Retry if sequence changed during copy
        
        return copy;
    }

private:
    std::unordered_map<std::string, TickMessage> snapshots_;
    alignas(64) std::atomic<size_t> sequence_;
};

class Worker {


public:
    Worker(BoundedQueue& queue, SnapshotStore& store) 
        : queue_(queue), store_(store) {}

    void run() {
        TickMessage msg;
        while (queue_.pop(msg)) {
            store_.update(msg);
        }
    }

private:
    BoundedQueue& queue_;
    SnapshotStore& store_;
};
