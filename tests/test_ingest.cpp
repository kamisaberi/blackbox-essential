#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>

#include "ingest/network_ingest.hpp"
#include "ingest/log_ingest.hpp"
#include "ingest/vision_ingest.hpp"

int main() {
    std::cout << "[Test Ingest] Starting full ingestion streamer verification..." << std::endl;

    std::vector<blackbox::SecurityEvent> received_events;
    std::mutex event_mutex;

    auto callback = [&](const blackbox::SecurityEvent& event) {
        std::lock_guard<std::mutex> lock(event_mutex);
        received_events.push_back(event);
    };

    // 1. Initialize Ingestion Drivers
    blackbox::ingest::NetworkIngest net_ingest("eth0");
    blackbox::ingest::LogIngest log_ingest(514);
    blackbox::ingest::VisionIngest vision_ingest("rtsp://127.0.0.1:8554/live");

    // 2. Start Async Streamers
    net_ingest.start(callback);
    log_ingest.start(callback);
    vision_ingest.start(callback);

    // 3. Allow streamers to produce data for 1.5 seconds
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 4. Stop Streamers
    net_ingest.stop();
    log_ingest.stop();
    vision_ingest.stop();

    // 5. Assert Received Events
    std::lock_guard<std::mutex> lock(event_mutex);
    std::cout << "[Test Ingest] Captured " << received_events.size() << " security events." << std::endl;
    
    assert(!received_events.empty());
    assert(received_events[0].event_id > 0);
    assert(!received_events[0].features.empty());

    bool found_network = false, found_log = false, found_vision = false;
    for (const auto& ev : received_events) {
        if (ev.type == blackbox::EventType::NetworkPacket) found_network = true;
        if (ev.type == blackbox::EventType::SystemLog) found_log = true;
        if (ev.type == blackbox::EventType::VisionCamera) found_vision = true;
    }

    assert(found_network && found_log && found_vision);
    std::cout << "[PASS] Ingestion drivers successfully verified across all modalities." << std::endl;

    return 0;
}