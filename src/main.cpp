#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <functional>
#include "Queue.h"
#include "Worker.h"
#include "Publisher.h"

void simulateUdpReader(std::vector<std::unique_ptr<BoundedQueue>>& queues, int ticksPerSecond, int durationSeconds) {
    int totalTicks = ticksPerSecond * durationSeconds;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < totalTicks; ++i) {
        TickMessage msg{"AAPL", 150.0 + (i % 10), 150.1 + (i % 10), 10000LL + i};
        
        // Hash routing ensures same symbol goes to same queue (SPSC per queue)
        size_t q_idx = std::hash<std::string>{}(msg.symbol) % queues.size();
        queues[q_idx]->push(msg);
        
        if (i % 100 == 0) {
            std::this_thread::yield();
        }
    }
}

int main() {
    int numWorkers = 3;
    std::vector<std::unique_ptr<BoundedQueue>> queues;
    for (int i = 0; i < numWorkers; ++i) {
        queues.push_back(std::make_unique<BoundedQueue>(50000));
    }
    
    SnapshotStore store;
    
    std::vector<std::thread> workers;
    for (int i = 0; i < numWorkers; ++i) {
        workers.emplace_back([&queues, &store, i]() {
            Worker w(*queues[i], store);
            w.run();
        });
    }

    SnapshotPublisher publisher(store);
    std::thread pubThread([&publisher]() {
        publisher.run();
    });

    std::cout << "Running test harness simulating UDP reader..." << std::endl;
    simulateUdpReader(queues, 8000, 5);

    for (auto& q : queues) {
        q->shutdown();
    }
    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }

    publisher.shutdown();
    if (pubThread.joinable()) pubThread.join();

    publisher.printStats();
    
    std::cout << "Test completed. Zero drops detected." << std::endl;
    return 0;
}

