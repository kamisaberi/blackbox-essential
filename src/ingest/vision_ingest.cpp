#include "vision_ingest.hpp"
#include <iostream>
#include <chrono>

namespace blackbox::ingest {

VisionIngest::VisionIngest(std::string rtsp_url) : rtsp_url_(std::move(rtsp_url)) {}

VisionIngest::~VisionIngest() {
    stop();
}

void VisionIngest::start(EventCallback callback) {
    running_ = true;
    vision_thread_ = std::thread([this, callback]() {
        uint64_t counter = 0;
        while (running_) {
            counter++;
            SecurityEvent event;
            event.event_id = counter + 20000;
            event.timestamp = std::chrono::system_clock::now();
            event.type = EventType::VisionCamera;
            event.description = "Camera Stream Frame # " + std::to_string(counter);
            event.features = {0.05f, 0.95f, 0.88f, 0.12f};

            callback(event);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
    std::cout << "[Ingest Vision] Processing RTSP video stream: " << rtsp_url_ << std::endl;
}

void VisionIngest::stop() {
    running_ = false;
    if (vision_thread_.joinable()) {
        vision_thread_.join();
    }
}

} // namespace blackbox::ingest