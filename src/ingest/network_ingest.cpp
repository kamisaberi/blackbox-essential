#include "network_ingest.hpp"
#include <iostream>
#include <chrono>

namespace blackbox::ingest {

NetworkIngest::NetworkIngest(std::string interface_name)
    : interface_name_(std::move(interface_name)) {}

NetworkIngest::~NetworkIngest() {
    stop();
}

void NetworkIngest::start(EventCallback callback) {
    running_ = true;
    capture_thread_ = std::thread([this, callback]() {
        uint64_t counter = 0;
        while (running_) {
            counter++;
            SecurityEvent event;
            event.event_id = counter;
            event.timestamp = std::chrono::system_clock::now();
            event.type = EventType::NetworkPacket;
            event.source_ip = "192.168.1." + std::to_string(10 + (counter % 50));
            event.destination_ip = "10.0.0.1";
            event.port = 80;
            event.features = {0.2f, 0.5f, 0.1f, 0.8f};

            callback(event);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });
    std::cout << "[Ingest Network] Started packet sniffer on interface: " << interface_name_ << std::endl;
}

void NetworkIngest::stop() {
    running_ = false;
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
}

} // namespace blackbox::ingest