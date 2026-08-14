#pragma once
#include "blackbox/event.hpp"
#include <atomic>
#include <thread>
#include <functional>

namespace blackbox::ingest {

class LogIngest {
public:
    using EventCallback = std::function<void(const SecurityEvent&)>;

    explicit LogIngest(int syslog_port);
    ~LogIngest();

    void start(EventCallback callback);
    void stop();

private:
    int syslog_port_;
    std::atomic<bool> running_{false};
    std::thread listener_thread_;
};

} // namespace blackbox::ingest