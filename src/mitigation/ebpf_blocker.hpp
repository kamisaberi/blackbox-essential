#pragma once

#include "blackbox/export.hpp"
#include <string>
#include <unordered_set>
#include <mutex>
#include <vector>
#include <cstdint>

namespace blackbox::mitigation
{

    // Must match struct slice_policy in xdp_drop.c
    struct KernelSlicePolicy
    {
        uint32_t sst{0};              // 1=eMBB, 2=URLLC, 3=mMTC, 4=Enterprise
        uint32_t sd{0};               // Slice Differentiator
        uint32_t allowed_inner_ip{0}; // Wildcard = 0
        uint16_t allowed_ports[4]{0}; // Permitted industrial ports
        uint8_t isolation_level{0};   // 0 = Strict, 1 = Permissive
        uint8_t pad[3]{0};
        uint64_t packet_counter{0};
        uint64_t violation_counter{0};
    };

    class BLACKBOX_API EBPFBlocker
    {
    public:
        explicit EBPFBlocker(std::string interface_name = "ens33");
        ~EBPFBlocker();

        bool attach_xdp_program(const std::string &bpf_obj_path);
        void detach_xdp_program();

        bool block_ip(const std::string &ip_address);
        bool unblock_ip(const std::string &ip_address);

        // Idea D: Deception Honeynet Redirection API
        bool redirect_to_deception(const std::string &attacker_ip,
                                   const std::string &decoy_ip,
                                   uint16_t decoy_port = 0,
                                   const std::string &decoy_mac = "");
        bool clear_deception(const std::string &attacker_ip);
        uint64_t get_deception_hits(const std::string &attacker_ip);

        // Idea B: 5G/6G GTP-U Network Slicing APIs
        bool register_5g_slice(uint32_t teid, uint32_t sst, uint32_t sd,
                               const std::vector<uint16_t> &allowed_ports,
                               uint8_t isolation = 0,
                               const std::string &allowed_ip = "0.0.0.0");
        bool revoke_5g_slice(uint32_t teid);
        bool get_5g_slice_policy(uint32_t teid, KernelSlicePolicy &out_policy);
        uint64_t get_5g_slice_violations(uint32_t sst);

        // Registers an AF_XDP socket FD into xsks_map
        bool register_xsk_socket(uint32_t queue_id, int xsk_fd);

        int get_map_fd() const { return map_fd_; }
        int get_xsks_map_fd() const { return xsks_map_fd_; }
        int get_deception_map_fd() const { return deception_map_fd_; }
        int get_teid_slice_map_fd() const { return teid_slice_map_fd_; }

    private:
        std::string interface_name_;
        int ifindex_{0};
        int map_fd_{-1};
        int prog_fd_{-1};
        void *bpf_obj_{nullptr};
        std::unordered_set<std::string> blocked_ips_;
        std::unordered_set<std::string> deceived_ips_;
        std::mutex blocker_mutex_;
        int xsks_map_fd_{-1};
        int deception_map_fd_{-1};
        int teid_slice_map_fd_{-1};
        int slice_drop_map_fd_{-1};
    };

} // namespace blackbox::mitigation