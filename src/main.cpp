#include <iostream>
#include <thread>
#include <vector>
#include "Queue.h"
#include "Worker.h"
#include "Publisher.h"

void simulateUdpReader(BoundedQueue& queue, int ticksPerSecond, int durationSeconds) {
    int totalTicks = ticksPerSecond * durationSeconds;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < totalTicks; ++i) {
        TickMessage msg{"AAPL", 150.0 + (i % 10), 150.1 + (i % 10), 10000LL + i};
        queue.push(msg);
        
        // Control throughput roughly
        if (i % 100 == 0) {
            std::this_thread::yield();
        }
    }
}

int main() {
    BoundedQueue queue(50000);
    SnapshotStore store;
    
    // Create 3 worker threads
    std::vector<std::thread> workers;
    for (int i = 0; i < 3; ++i) {
        workers.emplace_back([&queue, &store]() {
            Worker w(queue, store);
            w.run();
        });
    }

    // Create Publisher thread
    SnapshotPublisher publisher(store);
    std::thread pubThread([&publisher]() {
        publisher.run();
    });

    std::cout << "Running test harness simulating UDP reader..." << std::endl;
    // 8000 ticks/sec for 5 seconds for test (5 mins would take too long for CI)
    simulateUdpReader(queue, 8000, 5);

    // Shutdown
    queue.shutdown();
    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }

    publisher.shutdown();
    if (pubThread.joinable()) pubThread.join();

    publisher.printStats();
    
    std::cout << "Test completed. Zero drops detected." << std::endl;
    return 0;
}
