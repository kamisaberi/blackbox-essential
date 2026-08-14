#include <iostream>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <vector>

#include "blackbox/event.hpp"
#include "storage/ring_buffer.hpp"
#include "ai/ai_manager.hpp"
#include "correlator/rules_engine.hpp"
#include "mitigation/ebpf_blocker.hpp"

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " BLACKBOX: Microsecond SIEM End-to-End Latency Benchmarker" << std::endl;
    std::cout << "==========================================================" << std::endl;

    const int iterations = 50000;
    std::vector<double> latencies_us;
    latencies_us.reserve(iterations);

    blackbox::storage::EventRingBuffer ring_buffer(1024);
    blackbox::ai::AIManager ai_manager(xinfer::Target::TensorRT, "models/threat_detector.engine");
    blackbox::mitigation::EBPFBlocker ebpf_blocker;
    blackbox::correlator::RulesEngine rules_engine(ebpf_blocker);

    std::cout << "Executing " << iterations << " event iterations (Ingest -> AI -> Correlation -> Mitigation)..." << std::endl;

    for (int i = 0; i < iterations; ++i) {
        blackbox::SecurityEvent event;
        event.event_id = i + 1;
        event.timestamp = std::chrono::system_clock::now();
        event.type = blackbox::EventType::NetworkPacket;
        event.source_ip = "192.168.1.100";
        event.features = {0.1f, 0.4f, 0.95f, 0.2f};
        event.anomaly_score = (i % 100 == 0) ? 0.92f : 0.15f; // Periodic high anomaly

        auto t1 = std::chrono::high_resolution_clock::now();

        // 1. Ring Buffer Push & Pop
        ring_buffer.push(event);
        auto popped_opt = ring_buffer.pop();
        
        if (popped_opt.has_value()) {
            auto current_event = popped_opt.value();
            
            // 2. AI Inference Analysis (xInfer Engine)
            current_event.anomaly_score = ai_manager.analyze_event(current_event);
            
            // 3. Correlation & Kernel Mitigation
            rules_engine.process_event(current_event);
        }

        auto t2 = std::chrono::high_resolution_clock::now();
        double duration_us = std::chrono::duration<double, std::micro>(t2 - t1).count();
        latencies_us.push_back(duration_us);
    }

    // Calculate Percentile Statistics
    std::sort(latencies_us.begin(), latencies_us.end());

    double sum = std::accumulate(latencies_us.begin(), latencies_us.end(), 0.0);
    double avg = sum / latencies_us.size();
    double min_val = latencies_us.front();
    double max_val = latencies_us.back();
    double p95 = latencies_us[static_cast<size_t>(iterations * 0.95)];
    double p99 = latencies_us[static_cast<size_t>(iterations * 0.99)];
    double throughput_eps = 1000000.0 / avg;

    std::cout << "----------------------------------------------------------" << std::endl;
    std::cout << "Average Latency : " << avg << " microseconds (us)" << std::endl;
    std::cout << "Minimum Latency : " << min_val << " microseconds (us)" << std::endl;
    std::cout << "Maximum Latency : " << max_val << " microseconds (us)" << std::endl;
    std::cout << "P95 Latency     : " << p95 << " microseconds (us)" << std::endl;
    std::cout << "P99 Latency     : " << p99 << " microseconds (us)" << std::endl;
    std::cout << "Throughput      : " << throughput_eps << " Events/Sec (EPS)" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}