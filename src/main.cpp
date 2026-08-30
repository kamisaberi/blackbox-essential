#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

#include "blackbox/event.hpp"
#include "storage/ring_buffer.hpp"
#include "ai/ai_manager.hpp"
#include "correlator/rules_engine.hpp"
#include "mitigation/ebpf_blocker.hpp"

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[Blackbox Daemon] Shutting down gracefully..." << std::endl;
        g_running = false;
    }
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "==========================================================" << std::endl;
    std::cout << " BLACKBOX: Autonomous Air-Gapped Cyber-Physical Security  " << std::endl;
    std::cout << " Powered by xInfer Essential Engine (libxinfer.so)        " << std::endl;
    std::cout << " Operating System: Ubuntu Linux                           " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // 1. Initialize Subsystems (Use EventRingBuffer & Target::OpenVINO for CPU/VMware)
    blackbox::storage::EventRingBuffer event_queue(1024);
    blackbox::ai::AIManager ai_manager(xinfer::Target::OpenVINO, "threat_detector.onnx");
    blackbox::mitigation::EBPFBlocker ebpf_blocker;
    blackbox::correlator::RulesEngine rules_engine(ebpf_blocker);

    // 2. Worker Thread: High-Speed Event Processing & Mitigation Loop
    std::thread processing_thread([&]() {
        while (g_running) {
            auto event_opt = event_queue.pop();
            if (event_opt.has_value()) {
                auto event = event_opt.value();
                
                // Run xInfer Engine AI Inference
                event.anomaly_score = ai_manager.analyze_event(event);
                
                // Correlate and execute autonomous eBPF kernel mitigation
                rules_engine.process_event(event);
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    });

    // 3. Simulated Telemetry Ingestion Loop (Packet & Log Streamer)
    uint64_t event_counter = 0;
    std::cout << "[Blackbox Daemon] Active and monitoring incoming streams...\n" << std::endl;

    while (g_running) {
        event_counter++;
        
        blackbox::SecurityEvent event;
        event.event_id = event_counter;
        event.timestamp = std::chrono::system_clock::now();
        event.type = blackbox::EventType::NetworkPacket;
        event.source_ip = "192.168.1." + std::to_string(100 + (event_counter % 20));
        event.destination_ip = "10.0.0.1";
        event.port = 80;
        event.features = {0.1f, 0.4f, 0.9f, 0.2f};

        // Simulate periodic high-threat anomaly every 10 events
        if (event_counter % 10 == 0) {
            event.anomaly_score = 0.92f; // High anomaly trigger
            event.source_ip = "192.168.1.250"; // Attacker IP
        } else {
            event.anomaly_score = 0.12f;
        }

        event_queue.push(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (processing_thread.joinable()) {
        processing_thread.join();
    }

    std::cout << "[Blackbox Daemon] Stopped successfully." << std::endl;
    return 0;
}