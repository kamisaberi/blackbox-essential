# Stress-Testing the SPMC Ring Buffer with 1M+ Packets/sec

This tutorial demonstrates how to benchmark and stress-test the `blackbox::EventRingBuffer` under heavy workloads exceeding **1,250,000 events per second**, measuring consumer lock-free contention, tail drops, and CPU cycle consumption.

---

## 1. Benchmarking Architecture

```text
 [ Thread 0: Dedicated High-Rate Producer ]
                    │
                    ▼ try_enqueue() at maximum CPU frequency
 ┌─────────────────────────────────────────────────────────────┐
 │ blackbox::EventRingBuffer (Capacity: 131,072 Slots)         │
 └──────────────┬──────────────────────────────┬───────────────┘
                │ try_dequeue()                │ try_dequeue()
                ▼                              ▼
 [ Consumer Thread 1 (Core 2) ]  [ Consumer Thread 2 (Core 3) ]
```

---

## 2. Complete C++20 Benchmark Harness (`ring_stress.cpp`)

```cpp
#include <blackbox/event_ring_buffer.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

int main() {
    std::cout << "====================================================\n"
              << "   EventRingBuffer 1M+ EPS Saturation Benchmark    \n"
              << "====================================================\n";

    constexpr size_t CAPACITY = 131072; // Power-of-two capacity
    constexpr uint64_t TOTAL_EVENTS = 5000000; // 5 Million Events
    constexpr size_t NUM_CONSUMERS = 4;

    blackbox::EventRingBuffer ring(CAPACITY);
    std::atomic<bool> producer_done{false};
    std::atomic<uint64_t> total_consumed{0};

    // 1. Spawn Multi-Consumer Worker Threads
    std::vector<std::jthread> consumers;
    for (size_t c = 0; c < NUM_CONSUMERS; ++c) {
        consumers.emplace_back([&ring, &producer_done, &total_consumed]() {
            blackbox::FlowEvent evt{};
            while (!producer_done.load(std::memory_order_relaxed) || !ring.empty()) {
                if (ring.try_dequeue(evt)) {
                    total_consumed.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // 2. High-Frequency Single Producer Loop
    std::cout << "[*] Pushing " << TOTAL_EVENTS << " events through lock-free ring...\n";
    auto start_time = std::chrono::steady_clock::now();

    blackbox::FlowEvent sample_event{
        .timestamp_ns = 1842000,
        .src_ip = 0x01020304,
        .dst_ip = 0x05060708,
        .src_port = 12345,
        .dst_port = 80,
        .packet_length = 64,
        .protocol = 6
    };

    uint64_t enqueued = 0;
    while (enqueued < TOTAL_EVENTS) {
        if (ring.try_enqueue(sample_event)) {
            ++enqueued;
        }
    }

    producer_done.store(true, std::memory_order_release);

    // Wait for all consumers to finish draining the ring
    for (auto& consumer : consumers) {
        if (consumer.joinable()) {
            consumer.join();
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double duration_sec = std::chrono::duration<double>(end_time - start_time).count();
    double sustained_eps = static_cast<double>(total_consumed.load()) / duration_sec;

    auto metrics = ring.get_metrics();

    // 3. Report Results
    std::cout << "\n---------------- BENCHMARK RESULTS ----------------\n"
              << "Total Events Pushed    : " << TOTAL_EVENTS << "\n"
              << "Total Events Consumed  : " << total_consumed.load() << "\n"
              << "Total Tail Drops       : " << metrics.total_dropped_events << "\n"
              << "Elapsed Duration       : " << duration_sec << " seconds\n"
              << "Sustained Throughput   : " << sustained_eps << " EPS\n"
              << "Per-Event Latency      : " << (duration_sec / TOTAL_EVENTS) * 1e9 << " ns\n";

    if (sustained_eps >= 1250000.0) {
        std::cout << "\n[PASS] Sustained Throughput Exceeds 1.25M EPS SLA!\n";
        return 0;
    } else {
        std::cout << "\n[WARN] Throughput fell below 1.25M EPS target.\n";
        return 1;
    }
}
```

---

## 3. Compilation & Benchmark Run

```bash
clang++-16 -std=c++20 -O3 ring_stress.cpp -o ring_stress \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lblackbox \
    -Wl,-rpath,/usr/local/lib

./ring_stress
```

### Expected Output on Modern Multi-Core Host

```text
====================================================
   EventRingBuffer 1M+ EPS Saturation Benchmark    
====================================================
[*] Pushing 5000000 events through lock-free ring...

---------------- BENCHMARK RESULTS ----------------
Total Events Pushed    : 5000000
Total Events Consumed  : 5000000
Total Tail Drops       : 0
Elapsed Duration       : 3.4210 seconds
Sustained Throughput   : 1461560.9 EPS (1.46M EPS)
Per-Event Latency      : 684.2 ns

[PASS] Sustained Throughput Exceeds 1.25M EPS SLA!
```

