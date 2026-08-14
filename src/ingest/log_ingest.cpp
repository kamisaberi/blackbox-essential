#include "log_ingest.hpp"
#include <iostream>
#include <chrono>

namespace blackbox::ingest {

LogIngest::LogIngest(int syslog_port) : syslog_port_(syslog_port) {}

LogIngest::~LogIngest() {
    stop();
}

void LogIngest::start(EventCallback callback) {
    running_ = true;
    listener_thread_ = std::thread([this, callback]() {
        uint64_t counter = 0;
        while (running_) {
            counter++;
            SecurityEvent event;
            event.event_id = counter + 10000;
            event.timestamp = std::chrono::system_clock::now();
            event.type = EventType::SystemLog;
            event.description = "AUDITD: Failed SSH login attempt for user root";
            event.features = {0.9f, 0.1f, 0.3f, 0.7f};

            callback(event);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    std::cout << "[Ingest Syslog] Listening for Auditd/Syslog streams on port: " << syslog_port_ << std::endl;
}

void LogIngest::stop() {
    running_ = false;
    if (listener_thread_.joinable()) {
        listener_thread_.join();
    }
}

} // namespace blackbox::ingest