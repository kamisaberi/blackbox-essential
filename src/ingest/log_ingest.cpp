#include "log_ingest.hpp"
#include <iostream>

namespace blackbox::ingest {

LogIngest::LogIngest(storage::LockFreeRingBuffer<SecurityEvent, 1024>& queue) : queue_(queue) {}

void LogIngest::process_syslog_line(const std::string& line) {
    if (line.find("Failed password") != std::string::npos || line.find("Accepted password") != std::string::npos) {
        SecurityEvent event;
        event.timestamp = std::chrono::system_clock::now();
        event.type = EventType::SystemLog;
        event.description = line;
        event.anomaly_score = (line.find("Failed") != std::string::npos) ? 0.75f : 0.05f;

        queue_.push(event);
    }
}

} // namespace blackbox::ingest