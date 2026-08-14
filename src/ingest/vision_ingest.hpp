#pragma once
#include "blackbox/event.hpp"
#include <atomic>
#include <thread>
#include <functional>

namespace blackbox::ingest {

class VisionIngest {
public:
    using EventCallback = std::function<void(const SecurityEvent&)>;

    explicit VisionIngest(std::string rtsp_url);
    ~VisionIngest();

    void start(EventCallback callback);
    void stop();

private:
    std::string rtsp_url_;
    std::atomic<bool> running_{false};
    std::thread vision_thread_;
};

} // namespace blackbox::ingest