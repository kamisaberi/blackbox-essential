#include "ebpf_blocker.hpp"
#include <iostream>
#include <cstdlib>

namespace blackbox::mitigation {

bool EBPFBlocker::block_ip(const std::string& ip_address) {
    std::lock_guard<std::mutex> lock(blocker_mutex_);
    
    if (blocked_ips_.find(ip_address) != blocked_ips_.end()) {
        return true; // Already blocked
    }

    std::cout << "[eBPF Mitigation] >>> EXECUTING KERNEL XDP DROP FOR MALICIOUS IP: " 
              << ip_address << " <<<" << std::endl;

    // Call Linux iptables / eBPF kernel command on Ubuntu
    std::string cmd = "sudo iptables -A INPUT -s " + ip_address + " -j DROP 2>/dev/null";
    int res = std::system(cmd.c_str());

    blocked_ips_.insert(ip_address);
    return (res == 0);
}

bool EBPFBlocker::unblock_ip(const std::string& ip_address) {
    std::lock_guard<std::mutex> lock(blocker_mutex_);
    blocked_ips_.erase(ip_address);
    std::string cmd = "sudo iptables -D INPUT -s " + ip_address + " -j DROP 2>/dev/null";
    return (std::system(cmd.c_str()) == 0);
}

} // namespace blackbox::mitigation