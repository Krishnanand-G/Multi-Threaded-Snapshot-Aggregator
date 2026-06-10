#pragma once
#include <unordered_map>
#include <mutex>
#include "Queue.h"

class SnapshotStore {
public:
    void update(const TickMessage& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshots_[msg.symbol] = msg;
    }

    std::unordered_map<std::string, TickMessage> getSnapshots() {
        std::lock_guard<std::mutex> lock(mutex_);
        return snapshots_;
    }

private:
    std::unordered_map<std::string, TickMessage> snapshots_;
    std::mutex mutex_;
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
