#include "ebpf_blocker.hpp"
#include <iostream>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_link.h>
#include <cstring>
#include <filesystem>
#include <vector>

namespace blackbox::mitigation
{

    struct KernelDeceptionTarget {
        uint32_t decoy_ip;
        uint16_t decoy_port;
        uint8_t  decoy_mac[6];
        uint8_t  rewrite_mac;
        uint8_t  pad;
        uint64_t hit_counter;
    };

    EBPFBlocker::EBPFBlocker(std::string interface_name)
        : interface_name_(std::move(interface_name))
    {
        ifindex_ = if_nametoindex(interface_name_.c_str());
        if (ifindex_ == 0)
        {
            std::vector<std::string> fallbacks = {"ens160", "ens33", "eth0", "lo"};
            for (const auto &iface : fallbacks)
            {
                int idx = if_nametoindex(iface.c_str());
                if (idx != 0)
                {
                    ifindex_ = idx;
                    interface_name_ = iface;
                    break;
                }
            }
        }
        attach_xdp_program("src/mitigation/xdp_drop.o");
    }

    EBPFBlocker::~EBPFBlocker()
    {
        detach_xdp_program();
    }

    bool EBPFBlocker::attach_xdp_program(const std::string &bpf_obj_path)
    {
        if (ifindex_ == 0)
            return false;

        std::vector<std::string> search_paths = {
            bpf_obj_path,
            "../" + bpf_obj_path,
            "/home/kami/blackbox-essential/src/mitigation/xdp_drop.o",
            "/usr/local/lib/blackbox/xdp_drop.o",
            "/etc/sentinel/xdp_drop.o"};

        std::string resolved_path = bpf_obj_path;
        bool found = false;
        for (const auto &p : search_paths)
        {
            if (std::filesystem::exists(p))
            {
                resolved_path = p;
                found = true;
                break;
            }
        }

        if (!found)
        {
            std::cerr << "[eBPF Blocker] Error: xdp_drop.o not found in any standard path." << std::endl;
            return false;
        }

        struct bpf_object *obj = bpf_object__open_file(resolved_path.c_str(), nullptr);
        if (!obj)
        {
            std::cerr << "[eBPF Blocker] Failed to open BPF object: " << resolved_path << std::endl;
            return false;
        }

        if (bpf_object__load(obj) < 0)
        {
            std::cerr << "[eBPF Blocker] Failed to load BPF object into kernel." << std::endl;
            bpf_object__close(obj);
            return false;
        }

        struct bpf_program *prog = bpf_object__find_program_by_name(obj, "xdp_firewall");
        prog_fd_ = bpf_program__fd(prog);
        map_fd_ = bpf_object__find_map_fd_by_name(obj, "blocked_ip_map");
        xsks_map_fd_ = bpf_object__find_map_fd_by_name(obj, "xsks_map");
        deception_map_fd_ = bpf_object__find_map_fd_by_name(obj, "deception_map");
        teid_slice_map_fd_ = bpf_object__find_map_fd_by_name(obj, "teid_slice_map");
        slice_drop_map_fd_ = bpf_object__find_map_fd_by_name(obj, "slice_drop_map");
        bpf_obj_ = obj;

        unsigned int xdp_flags = XDP_FLAGS_SKB_MODE;
        if (bpf_xdp_attach(ifindex_, prog_fd_, xdp_flags, nullptr) < 0)
        {
            std::cerr << "[eBPF Blocker Warning] Failed to attach XDP to interface " << interface_name_ << std::endl;
            return false;
        }

        std::cout << "[eBPF Kernel Driver] Attached native XDP filter (" << resolved_path << ") to: "
                  << interface_name_ << " (5G Slicing & Deception active)." << std::endl;
        return true;
    }

    void EBPFBlocker::detach_xdp_program()
    {
        if (ifindex_ > 0 && prog_fd_ > 0)
        {
            bpf_xdp_detach(ifindex_, XDP_FLAGS_SKB_MODE, nullptr);
            std::cout << "[eBPF Kernel Driver] Detached XDP filter from: " << interface_name_ << std::endl;
        }
        if (bpf_obj_)
        {
            bpf_object__close(static_cast<struct bpf_object *>(bpf_obj_));
            bpf_obj_ = nullptr;
        }
    }

    bool EBPFBlocker::register_xsk_socket(uint32_t queue_id, int xsk_fd)
    {
        if (xsks_map_fd_ < 0 || xsk_fd < 0)
            return false;

        int ret = bpf_map_update_elem(xsks_map_fd_, &queue_id, &xsk_fd, BPF_ANY);
        return (ret == 0);
    }

    bool EBPFBlocker::block_ip(const std::string &ip_address)
    {
        std::lock_guard<std::mutex> lock(blocker_mutex_);
        if (blocked_ips_.find(ip_address) != blocked_ips_.end())
            return true;

        struct in_addr addr;
        if (inet_pton(AF_INET, ip_address.c_str(), &addr) != 1)
            return false;

        uint32_t key = addr.s_addr;
        uint64_t initial_counter = 0;

        if (map_fd_ >= 0)
        {
            int res = bpf_map_update_elem(map_fd_, &key, &initial_counter, BPF_ANY);
            if (res == 0)
            {
                blocked_ips_.insert(ip_address);
                return true;
            }
        }
        return false;
    }

    bool EBPFBlocker::unblock_ip(const std::string &ip_address)
    {
        std::lock_guard<std::mutex> lock(blocker_mutex_);
        struct in_addr addr;
        if (inet_pton(AF_INET, ip_address.c_str(), &addr) != 1)
            return false;

        uint32_t key = addr.s_addr;
        if (map_fd_ >= 0)
        {
            bpf_map_delete_elem(map_fd_, &key);
        }
        blocked_ips_.erase(ip_address);
        return true;
    }

    bool EBPFBlocker::redirect_to_deception(const std::string &attacker_ip,
                                           const std::string &decoy_ip,
                                           uint16_t decoy_port,
                                           const std::string &decoy_mac)
    {
        std::lock_guard<std::mutex> lock(blocker_mutex_);
        if (deception_map_fd_ < 0)
            return false;

        struct in_addr src_addr, dst_addr;
        if (inet_pton(AF_INET, attacker_ip.c_str(), &src_addr) != 1 ||
            inet_pton(AF_INET, decoy_ip.c_str(), &dst_addr) != 1)
            return false;

        KernelDeceptionTarget target{};
        target.decoy_ip = dst_addr.s_addr;
        target.decoy_port = (decoy_port != 0) ? htons(decoy_port) : 0;
        target.hit_counter = 0;

        if (!decoy_mac.empty())
        {
            unsigned int mac[6];
            if (sscanf(decoy_mac.c_str(), "%x:%x:%x:%x:%x:%x",
                       &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6)
            {
                for (int i = 0; i < 6; ++i)
                    target.decoy_mac[i] = static_cast<uint8_t>(mac[i]);
                target.rewrite_mac = 1;
            }
        }

        uint32_t key = src_addr.s_addr;
        int res = bpf_map_update_elem(deception_map_fd_, &key, &target, BPF_ANY);
        if (res == 0)
        {
            deceived_ips_.insert(attacker_ip);
            return true;
        }
        return false;
    }

    bool EBPFBlocker::clear_deception(const std::string &attacker_ip)
    {
        std::lock_guard<std::mutex> lock(blocker_mutex_);
        if (deception_map_fd_ < 0)
            return false;

        struct in_addr addr;
        if (inet_pton(AF_INET, attacker_ip.c_str(), &addr) != 1)
            return false;

        uint32_t key = addr.s_addr;
        bpf_map_delete_elem(deception_map_fd_, &key);
        deceived_ips_.erase(attacker_ip);
        return true;
    }

    uint64_t EBPFBlocker::get_deception_hits(const std::string &attacker_ip)
    {
        std::lock_guard<std::mutex> lock(blocker_mutex_);
        if (deception_map_fd_ < 0)
            return 0;

        struct in_addr addr;
        if (inet_pton(AF_INET, attacker_ip.c_str(), &addr) != 1)
            return 0;

        uint32_t key = addr.s_addr;
        KernelDeceptionTarget target{};
        if (bpf_map_lookup_elem(deception_map_fd_, &key, &target) == 0)
            return target.hit_counter;
        return 0;
    }

    // -------------------------------------------------------------------------
    // IDEA B: 5G/6G NETWORK SLICING APIS
    // -------------------------------------------------------------------------
    bool EBPFBlocker::register_5g_slice(uint32_t teid, uint32_t sst, uint32_t sd,
                                       const std::vector<uint16_t> &allowed_ports,
                                       uint8_t isolation,
                                       const std::string &allowed_ip)
    {
        std::lock_guard<std::mutex> lock(blocker_mutex_);
        if (teid_slice_map_fd_ < 0)
            return false;

        KernelSlicePolicy policy{};
        policy.sst = sst;
        policy.sd = sd;
        policy.isolation_level = isolation;

        if (!allowed_ip.empty() && allowed_ip != "0.0.0.0")
        {
            struct in_addr addr;
            if (inet_pton(AF_INET, allowed_ip.c_str(), &addr) == 1)
                policy.allowed_inner_ip = addr.s_addr;
        }

        for (size_t i = 0; i < allowed_ports.size() && i < 4; ++i)
        {
            policy.allowed_ports[i] = allowed_ports[i];
        }

        int res = bpf_map_update_elem(teid_slice_map_fd_, &teid, &policy, BPF_ANY);
        if (res == 0)
        {
            std::cout << "[eBPF 5G Core] Enrolled Slice: TEID 0x" << std::hex << teid
                      << " -> SST " << std::dec << sst << " (SD: " << sd << ")" << std::endl;
            return true;
        }
        return false;
    }

    bool EBPFBlocker::revoke_5g_slice(uint32_t teid)
    {
        std::lock_guard<std::mutex> lock(blocker_mutex_);
        if (teid_slice_map_fd_ < 0)
            return false;

        int res = bpf_map_delete_elem(teid_slice_map_fd_, &teid);
        return (res == 0);
    }

    bool EBPFBlocker::get_5g_slice_policy(uint32_t teid, KernelSlicePolicy &out_policy)
    {
        std::lock_guard<std::mutex> lock(blocker_mutex_);
        if (teid_slice_map_fd_ < 0)
            return false;

        return (bpf_map_lookup_elem(teid_slice_map_fd_, &teid, &out_policy) == 0);
    }

    uint64_t EBPFBlocker::get_5g_slice_violations(uint32_t sst)
    {
        std::lock_guard<std::mutex> lock(blocker_mutex_);
        if (slice_drop_map_fd_ < 0)
            return 0;

        uint64_t count = 0;
        bpf_map_lookup_elem(slice_drop_map_fd_, &sst, &count);
        return count;
    }

} // namespace blackbox::mitigation