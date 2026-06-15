#pragma once
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include "Worker.h"

class SnapshotPublisher {
public:
    SnapshotPublisher(SnapshotStore& store) : store_(store), active_(true) {}

    void run() {
        while (active_) {
            auto start = std::chrono::high_resolution_clock::now();
            
            auto snaps = store_.getSnapshots();
            
            auto end = std::chrono::high_resolution_clock::now();
            auto latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            latencies_.push_back(latency_ns);
            
            // Publish snapshot (simulated)
            // std::cout << "Published " << snaps.size() << " symbols" << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void shutdown() {
        active_ = false;
    }

    void printStats() {
        if (latencies_.empty()) return;
        std::sort(latencies_.begin(), latencies_.end());
        size_t p50_idx = latencies_.size() * 0.50;
        size_t p99_idx = latencies_.size() * 0.99;
        
        std::cout << "Publisher Latency Stats:\n";
        std::cout << "p50: " << latencies_[p50_idx] << " ns\n";
        std::cout << "p99: " << latencies_[p99_idx] << " ns\n";
    }

private:
    SnapshotStore& store_;
    bool active_;
    std::vector<long long> latencies_;
};
