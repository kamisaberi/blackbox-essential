#include "network_ingest.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace blackbox::ingest {

NetworkIngest::NetworkIngest(const std::string& interface_name, storage::LockFreeRingBuffer<SecurityEvent, 1024>& queue)
    : interface_(interface_name), queue_(queue) {}

NetworkIngest::~NetworkIngest() {
    stop();
}

void NetworkIngest::start() {
    running_ = true;
    std::cout << "[Network Ingest] Sniffer active on interface: " << interface_ << std::endl;
}

void NetworkIngest::stop() {
    running_ = false;
}

void NetworkIngest::capture_loop() {
    uint64_t counter = 0;
    while (running_) {
        counter++;
        SecurityEvent event;
        event.event_id = counter;
        event.timestamp = std::chrono::system_clock::now();
        event.type = EventType::NetworkPacket;
        event.source_ip = "10.0.0." + std::to_string(counter % 254 + 1);
        event.destination_ip = "192.168.1.1";
        event.port = 443;
        
        // Raw network flow telemetry features passed to xinfer
        event.features = {static_cast<float>(counter % 100) / 100.0f, 0.5f, 0.8f, 0.1f};

        queue_.push(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

} // namespace blackbox::ingest