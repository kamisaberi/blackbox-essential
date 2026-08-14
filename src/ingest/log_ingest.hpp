#pragma once
#include "blackbox/event.hpp"
#include "storage/ring_buffer.hpp"
#include <string>

namespace blackbox::ingest {

class LogIngest {
public:
    explicit LogIngest(storage::LockFreeRingBuffer<SecurityEvent, 1024>& queue);
    ~LogIngest() = default;

    void process_syslog_line(const std::string& line);

private:
    storage::LockFreeRingBuffer<SecurityEvent, 1024>& queue_;
};

} // namespace blackbox::ingest