#pragma once
#include "export.hpp"
#include "event.hpp"
#include "types.hpp"
#include <memory>
#include <string>

namespace blackbox {

// High-level C++ Controller Class Exported in libblackbox.so
class BLACKBOX_API BlackboxEngine {
public:
    explicit BlackboxEngine(const std::string& config_file_path);
    ~BlackboxEngine();

    // Start / Stop autonomous security operations
    bool start();
    void stop();

    // Manual Event Ingestion & Mitigation Triggering
    void submit_event(const SecurityEvent& event);
    bool block_ip_address(const std::string& ip_address);
    bool unblock_ip_address(const std::string& ip_address);

    bool is_running() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace blackbox