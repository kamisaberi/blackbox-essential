#pragma once
#include <string>
#include <unordered_set>
#include <mutex>
#include "blackbox/export.hpp"  // <--- ADD THIS


namespace blackbox::mitigation {

class EBPFBlocker {
public:
    EBPFBlocker() = default;
    ~EBPFBlocker() = default;

    // Drops packets from malicious IP at kernel level
    bool block_ip(const std::string& ip_address);
    bool unblock_ip(const std::string& ip_address);

private:
    std::unordered_set<std::string> blocked_ips_;
    std::mutex blocker_mutex_;
};

} // namespace blackbox::mitigation