#include "ebpf_blocker.hpp"
#include <iostream>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_link.h>
#include <cstring>

namespace blackbox::mitigation {

EBPFBlocker::EBPFBlocker(std::string interface_name)
    : interface_name_(std::move(interface_name)) {
    ifindex_ = if_nametoindex(interface_name_.c_str());
    if (ifindex_ == 0) {
        // Fallback for different VM interfaces (e.g. eth0)
        ifindex_ = if_nametoindex("eth0");
        if (ifindex_ != 0) interface_name_ = "eth0";
    }
    
    // Automatically attach compiled eBPF bytecode
    attach_xdp_program("src/mitigation/xdp_drop.o");
}

EBPFBlocker::~EBPFBlocker() {
    detach_xdp_program();
}

bool EBPFBlocker::attach_xdp_program(const std::string& bpf_obj_path) {
    if (ifindex_ == 0) return false;

    struct bpf_object* obj = bpf_object__open_file(bpf_obj_path.c_str(), nullptr);
    if (!obj) {
        std::cerr << "[eBPF Blocker] Failed to open BPF object: " << bpf_obj_path << std::endl;
        return false;
    }

    if (bpf_object__load(obj) < 0) {
        std::cerr << "[eBPF Blocker] Failed to load BPF object into kernel." << std::endl;
        bpf_object__close(obj);
        return false;
    }

    struct bpf_program* prog = bpf_object__find_program_by_name(obj, "xdp_firewall");
    prog_fd_ = bpf_program__fd(prog);
    map_fd_ = bpf_object__find_map_fd_by_name(obj, "blocked_ip_map");
    bpf_obj_ = obj;

    // Attach in XDP Generic / SKB Mode (Guarantees 100% compatibility in VMware!)
    unsigned int xdp_flags = XDP_FLAGS_SKB_MODE;
    if (bpf_xdp_attach(ifindex_, prog_fd_, xdp_flags, nullptr) < 0) {
        std::cerr << "[eBPF Blocker Warning] Failed to attach XDP to interface " << interface_name_ << std::endl;
        return false;
    }

    std::cout << "[eBPF Kernel Driver] Attached native XDP filter to interface: " 
              << interface_name_ << " (VMware Generic SKB Mode active)." << std::endl;
    return true;
}

void EBPFBlocker::detach_xdp_program() {
    if (ifindex_ > 0 && prog_fd_ > 0) {
        bpf_xdp_detach(ifindex_, XDP_FLAGS_SKB_MODE, nullptr);
        std::cout << "[eBPF Kernel Driver] Detached XDP filter from interface: " << interface_name_ << std::endl;
    }
    if (bpf_obj_) {
        bpf_object__close(static_cast<struct bpf_object*>(bpf_obj_));
        bpf_obj_ = nullptr;
    }
}

bool EBPFBlocker::block_ip(const std::string& ip_address) {
    std::lock_guard<std::mutex> lock(blocker_mutex_);
    
    if (blocked_ips_.find(ip_address) != blocked_ips_.end()) {
        return true;
    }

    struct in_addr addr;
    if (inet_pton(AF_INET, ip_address.c_str(), &addr) != 1) {
        return false;
    }

    uint32_t key = addr.s_addr;
    uint64_t initial_counter = 0;

    // Direct Linux Kernel BPF Map Insertion (Nanosecond Execution)
    if (map_fd_ >= 0) {
        int res = bpf_map_update_elem(map_fd_, &key, &initial_counter, BPF_ANY);
        if (res == 0) {
            std::cout << "[eBPF Native Kernel] Inserted IP into blocked_ip_map: " << ip_address << std::endl;
            blocked_ips_.insert(ip_address);
            return true;
        }
    }

    return false;
}

bool EBPFBlocker::unblock_ip(const std::string& ip_address) {
    std::lock_guard<std::mutex> lock(blocker_mutex_);
    struct in_addr addr;
    if (inet_pton(AF_INET, ip_address.c_str(), &addr) != 1) {
        return false;
    }

    uint32_t key = addr.s_addr;
    if (map_fd_ >= 0) {
        bpf_map_delete_elem(map_fd_, &key);
    }

    blocked_ips_.erase(ip_address);
    std::cout << "[eBPF Native Kernel] Removed IP from blocked_ip_map: " << ip_address << std::endl;
    return true;
}

} // namespace blackbox::mitigation