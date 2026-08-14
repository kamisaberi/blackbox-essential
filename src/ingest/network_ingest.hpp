#pragma once
#include "blackbox/event.hpp"
#include "storage/ring_buffer.hpp"
#include <string>
#include <atomic>

namespace blackbox::ingest {

class NetworkIngest {
public:
    NetworkIngest(const std::string& interface_name, storage::LockFreeRingBuffer<SecurityEvent, 1024>& queue);
    ~NetworkIngest();

    void start();
    void stop();

private:
    void capture_loop();

    std::string interface_;
    storage::LockFreeRingBuffer<SecurityEvent, 1024>& queue_;
    std::atomic<bool> running_{false};
};

} // namespace blackbox::ingest