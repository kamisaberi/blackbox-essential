#pragma once

#include "blackbox/export.hpp"
#include <string>
#include <unordered_set>
#include <mutex>

namespace blackbox::mitigation
{

    class BLACKBOX_API EBPFBlocker
    {
    public:
        explicit EBPFBlocker(std::string interface_name = "ens33");
        ~EBPFBlocker();

        bool attach_xdp_program(const std::string &bpf_obj_path);
        void detach_xdp_program();

        bool block_ip(const std::string &ip_address);
        bool unblock_ip(const std::string &ip_address);

        // Registers an AF_XDP socket FD into xsks_map for zero-copy userspace redirection
        bool register_xsk_socket(uint32_t queue_id, int xsk_fd);

        int get_map_fd() const { return map_fd_; }
        int get_xsks_map_fd() const { return xsks_map_fd_; }

    private:
        std::string interface_name_;
        int ifindex_{0};
        int map_fd_{-1};
        int prog_fd_{-1};
        void *bpf_obj_{nullptr};
        std::unordered_set<std::string> blocked_ips_;
        std::mutex blocker_mutex_;
        int xsks_map_fd_{-1};
    };

} // namespace blackbox::mitigation