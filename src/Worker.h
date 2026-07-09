#pragma once
#include <unordered_map>
#include <atomic>
#include <thread>
#include "Queue.h"

class SnapshotStore {
public:
    void update(const TickMessage& msg) {
        while (lock_.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        snapshots_[msg.symbol] = msg;
        lock_.clear(std::memory_order_release);
    }

    std::unordered_map<std::string, TickMessage> getSnapshots() {
        while (lock_.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        auto copy = snapshots_;
        lock_.clear(std::memory_order_release);
        return copy;
    }

private:
    std::unordered_map<std::string, TickMessage> snapshots_;
    alignas(64) std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
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
