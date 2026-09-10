#pragma once
#include "export.hpp"
#include "event.hpp"
#include "types.hpp"
#include <memory>
#include <string>
#include <vector>

namespace blackbox {

class BLACKBOX_API BlackboxEngine {
public:
    explicit BlackboxEngine(const std::string& config_file_path);
    ~BlackboxEngine();

    bool start();
    void stop();

    void submit_event(const SecurityEvent& event);
    bool block_ip_address(const std::string& ip_address);
    bool unblock_ip_address(const std::string& ip_address);

    bool is_running() const;

    // REAL-TIME: Returns live threats detected in memory
    std::vector<SecurityEvent> get_recent_threats(size_t limit = 50);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace blackbox