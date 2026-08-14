#pragma once
#include "blackbox/event.hpp"
#include <atomic>
#include <thread>
#include <functional>

namespace blackbox::ingest {

class NetworkIngest {
public:
    using EventCallback = std::function<void(const SecurityEvent&)>;

    explicit NetworkIngest(std::string interface_name);
    ~NetworkIngest();

    void start(EventCallback callback);
    void stop();

private:
    std::string interface_name_;
    std::atomic<bool> running_{false};
    std::thread capture_thread_;
};

} // namespace blackbox::ingest