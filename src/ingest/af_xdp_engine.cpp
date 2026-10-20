#include "blackbox/af_xdp_engine.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <poll.h>
#include <net/if.h>
#include <sys/mman.h>
#include <linux/if_link.h>
#include <linux/if_xdp.h>

// Support both libxdp and libbpf header locations
#if __has_include(<xdp/xsk.h>)
#include <xdp/xsk.h>
#elif __has_include(<bpf/xsk.h>)
#include <bpf/xsk.h>
#endif

namespace blackbox::ingest
{

    AfXdpConfig AfXdpConfig::load_from_json(const std::string &json_path)
    {
        AfXdpConfig cfg{};

        // Multi-path search so binary finds the config file regardless of launch directory
        std::vector<std::string> search_paths = {
            json_path,
            "configs/af_xdp_config.json",
            "../configs/af_xdp_config.json",
            "blackbox-essential/configs/af_xdp_config.json",
            "/etc/sentinel/af_xdp_config.json"};

        std::ifstream file;
        std::string resolved_path;
        for (const auto &path : search_paths)
        {
            file.open(path);
            if (file.is_open())
            {
                resolved_path = path;
                break;
            }
        }

        if (!file.is_open())
        {
            std::cout << "[!] [AF_XDP Config] Config file not found at " << json_path
                      << ". Using VMware default fallback settings (ens160, Q0, XDP_COPY)." << std::endl;
            return cfg;
        }

        cfg.queue_ids.clear();
        std::string line;
        while (std::getline(file, line))
        {
            // 1. Parse interface_name
            if (line.find("interface_name") != std::string::npos)
            {
                auto pos = line.find("interface_name");
                auto first_q = line.find('"', pos + 14);
                if (first_q != std::string::npos)
                {
                    auto second_q = line.find('"', first_q + 1);
                    if (second_q != std::string::npos)
                    {
                        cfg.interface_name = line.substr(first_q + 1, second_q - first_q - 1);
                    }
                }
            }
            // 2. Parse queue_ids array or single queue_id
            else if (line.find("queue_ids") != std::string::npos || line.find("queue_id") != std::string::npos)
            {
                auto pos = line.find(':');
                if (pos != std::string::npos)
                {
                    std::string val = line.substr(pos + 1);
                    std::stringstream ss(val);
                    std::string token;
                    while (std::getline(ss, token, ','))
                    {
                        for (char &c : token)
                        {
                            if (c == '[' || c == ']' || c == '\"' || c == ' ')
                                c = ' ';
                        }
                        std::stringstream token_ss(token);
                        uint32_t q_num;
                        if (token_ss >> q_num)
                        {
                            cfg.queue_ids.push_back(q_num);
                        }
                    }
                }
            }
            // 3. Parse frame_size
            else if (line.find("frame_size") != std::string::npos)
            {
                auto pos = line.find(':');
                if (pos != std::string::npos)
                {
                    try { cfg.frame_size = std::stoul(line.substr(pos + 1)); } catch (...) {}
                }
            }
            // 4. Parse frame_count / num_frames
            else if (line.find("frame_count") != std::string::npos || line.find("num_frames") != std::string::npos)
            {
                auto pos = line.find(':');
                if (pos != std::string::npos)
                {
                    try { cfg.num_frames = std::stoul(line.substr(pos + 1)); } catch (...) {}
                }
            }
            // 5. Parse force_copy_mode / vmware_skb_fallback
            else if (line.find("vmware_skb_fallback") != std::string::npos || line.find("force_copy_mode") != std::string::npos)
            {
                if (line.find("true") != std::string::npos)
                {
                    cfg.force_copy_mode = true;
                }
                else if (line.find("false") != std::string::npos)
                {
                    cfg.force_copy_mode = false;
                }
            }
        }

        if (cfg.queue_ids.empty())
        {
            cfg.queue_ids.push_back(0);
        }
        cfg.queue_id = cfg.queue_ids[0];

        std::cout << "[+] [AF_XDP Config] Loaded dynamic config from " << resolved_path
                  << ": Interface=" << cfg.interface_name
                  << " | Queues=" << cfg.queue_ids.size()
                  << " | FrameSize=" << cfg.frame_size
                  << " | ForceCopy=" << (cfg.force_copy_mode ? "true" : "false") << std::endl;

        return cfg;
    }

    // Complete type definitions defined before constructor
    struct UmemContext
    {
        struct xsk_ring_prod fq;
        struct xsk_ring_cons cq;
        struct xsk_umem *umem{nullptr};
        void *buffer{nullptr};
    };

    struct SocketContext
    {
        struct xsk_ring_cons rx;
        struct xsk_ring_prod tx;
        struct xsk_socket *xsk{nullptr};
        int fd{-1};
        uint32_t queue_id{0};
    };

    AfXdpEngine::AfXdpEngine(AfXdpConfig config)
        : config_(std::move(config))
    {
        if (config_.queue_ids.empty())
        {
            config_.queue_ids.push_back(config_.queue_id);
        }
    }

    AfXdpEngine::~AfXdpEngine()
    {
        stop();
    }

    bool AfXdpEngine::allocate_umem()
    {
        umem_size_ = static_cast<size_t>(config_.num_frames) * config_.frame_size;

        if (posix_memalign(&umem_buffer_, getpagesize(), umem_size_) != 0)
        {
            std::cerr << "[-] [AF_XDP] Failed to allocate page-aligned UMEM memory ("
                      << umem_size_ << " bytes)" << std::endl;
            return false;
        }

        umem_info_ = std::make_unique<UmemContext>();
        umem_info_->buffer = umem_buffer_;

        struct xsk_umem_config u_cfg{};
        u_cfg.fill_size = config_.num_frames;
        u_cfg.comp_size = config_.num_frames / 2;
        u_cfg.frame_size = config_.frame_size;
        u_cfg.frame_headroom = 0;
        u_cfg.flags = 0;

        int ret = xsk_umem__create(&umem_info_->umem,
                                   umem_buffer_,
                                   umem_size_,
                                   &umem_info_->fq,
                                   &umem_info_->cq,
                                   &u_cfg);
        if (ret != 0)
        {
            std::cerr << "[-] [AF_XDP] xsk_umem__create failed: " << strerror(-ret) << std::endl;
            free(umem_buffer_);
            umem_buffer_ = nullptr;
            return false;
        }

        // Populate Fill Ring with initial empty UMEM chunks
        uint32_t idx = 0;
        ret = xsk_ring_prod__reserve(&umem_info_->fq, config_.num_frames / 2, &idx);
        if (ret > 0)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(ret); ++i)
            {
                *xsk_ring_prod__fill_addr(&umem_info_->fq, idx++) = i * config_.frame_size;
            }
            xsk_ring_prod__submit(&umem_info_->fq, ret);
        }

        return true;
    }

    bool AfXdpEngine::setup_xsk_socket(size_t socket_idx, uint32_t q_id, bool attempt_zero_copy)
    {
        auto sock_ctx = std::make_unique<SocketContext>();
        sock_ctx->queue_id = q_id;

        struct xsk_socket_config x_cfg{};
        x_cfg.rx_size = XSK_RING_CONS__DEFAULT_NUM_DESCS;
        x_cfg.tx_size = XSK_RING_PROD__DEFAULT_NUM_DESCS;
        x_cfg.libbpf_flags = XSK_LIBBPF_FLAGS__INHIBIT_PROG_LOAD;

        if (attempt_zero_copy)
        {
            x_cfg.bind_flags = XDP_ZEROCOPY;
            x_cfg.xdp_flags = XDP_FLAGS_DRV_MODE;
        }
        else
        {
            x_cfg.bind_flags = XDP_COPY;
            x_cfg.xdp_flags = XDP_FLAGS_SKB_MODE;
        }

        int ret = xsk_socket__create(&sock_ctx->xsk,
                                     config_.interface_name.c_str(),
                                     q_id,
                                     umem_info_->umem,
                                     &sock_ctx->rx,
                                     &sock_ctx->tx,
                                     &x_cfg);
        if (ret != 0)
        {
            return false;
        }

        sock_ctx->fd = xsk_socket__fd(sock_ctx->xsk);

        if (socket_idx < xsk_sockets_.size())
        {
            xsk_sockets_[socket_idx] = std::move(sock_ctx);
        }
        else
        {
            xsk_sockets_.push_back(std::move(sock_ctx));
        }

        return true;
    }

    bool AfXdpEngine::start()
    {
        if (running_.load())
            return true;

        if (!allocate_umem())
            return false;

        xsk_sockets_.clear();

        // Bind sockets across all configured hardware RSS queues
        for (size_t idx = 0; idx < config_.queue_ids.size(); ++idx)
        {
            uint32_t q_id = config_.queue_ids[idx];
            bool bound = false;

            if (!config_.force_copy_mode)
            {
                bound = setup_xsk_socket(idx, q_id, true);
                if (bound)
                {
                    zero_copy_active_ = true;
                }
            }

            if (!bound)
            {
                bound = setup_xsk_socket(idx, q_id, false);
                if (bound)
                {
                    zero_copy_active_ = false;
                }
                else
                {
                    std::cerr << "[-] [AF_XDP Engine] Failed to bind AF_XDP socket on "
                              << config_.interface_name << " (Queue " << q_id << ")" << std::endl;
                    stop();
                    return false;
                }
            }
        }

        if (zero_copy_active_)
        {
            std::cout << "\033[1;32m[+] [AF_XDP Engine] Multi-Queue RSS bound across "
                      << xsk_sockets_.size() << " queue(s) in HARDWARE ZERO-COPY mode (40M+ EPS capacity)\033[0m" << std::endl;
        }
        else
        {
            std::cout << "\033[1;33m[!] [AF_XDP Engine] Multi-Queue RSS bound across "
                      << xsk_sockets_.size() << " queue(s) in XDP_COPY mode (VMware/Docker fallback)\033[0m" << std::endl;
        }

        running_.store(true);
        return true;
    }

    void AfXdpEngine::stop()
    {
        if (!running_.load())
            return;
        running_.store(false);

        for (auto &sock_ctx : xsk_sockets_)
        {
            if (sock_ctx && sock_ctx->xsk)
            {
                xsk_socket__delete(sock_ctx->xsk);
                sock_ctx->xsk = nullptr;
            }
        }
        xsk_sockets_.clear();

        if (umem_info_ && umem_info_->umem)
        {
            xsk_umem__delete(umem_info_->umem);
            umem_info_->umem = nullptr;
        }
        if (umem_buffer_)
        {
            free(umem_buffer_);
            umem_buffer_ = nullptr;
        }
    }

    uint32_t AfXdpEngine::poll_queue(size_t queue_idx, uint32_t max_batch, PacketHandler handler)
    {
        if (!running_.load() || queue_idx >= xsk_sockets_.size() || !xsk_sockets_[queue_idx])
            return 0;

        auto &sock_ctx = xsk_sockets_[queue_idx];
        uint32_t idx_rx = 0;
        uint32_t rcvd = xsk_ring_cons__peek(&sock_ctx->rx, max_batch, &idx_rx);
        if (rcvd == 0)
            return 0;

        for (uint32_t i = 0; i < rcvd; ++i)
        {
            const struct xdp_desc *desc = xsk_ring_cons__rx_desc(&sock_ctx->rx, idx_rx++);
            uint64_t addr = desc->addr;
            uint32_t len = desc->len;

            const uint8_t *pkt_ptr = static_cast<const uint8_t *>(umem_buffer_) + addr;
            handler(pkt_ptr, len);
        }

        xsk_ring_cons__release(&sock_ctx->rx, rcvd);
        total_packets_.fetch_add(rcvd, std::memory_order_relaxed);

        // Replenish Fill Ring with newly available frames
        uint32_t idx_fq = 0;
        if (xsk_ring_prod__reserve(&umem_info_->fq, rcvd, &idx_fq) > 0)
        {
            for (uint32_t i = 0; i < rcvd; ++i) {
                *xsk_ring_prod__fill_addr(&umem_info_->fq, idx_fq++) = (i * config_.frame_size);
            }
            xsk_ring_prod__submit(&umem_info_->fq, rcvd);
        }

        return rcvd;
    }

    uint32_t AfXdpEngine::poll_batch(uint32_t max_batch, PacketHandler handler)
    {
        return poll_queue(0, max_batch, handler);
    }

    uint32_t AfXdpEngine::poll_all_queues(uint32_t max_batch_per_queue, PacketHandler handler)
    {
        uint32_t total_rcvd = 0;
        for (size_t q_idx = 0; q_idx < xsk_sockets_.size(); ++q_idx)
        {
            total_rcvd += poll_queue(q_idx, max_batch_per_queue, handler);
        }
        return total_rcvd;
    }

    int AfXdpEngine::get_xsk_fd(size_t queue_idx) const
    {
        if (queue_idx < xsk_sockets_.size() && xsk_sockets_[queue_idx])
        {
            return xsk_sockets_[queue_idx]->fd;
        }
        return -1;
    }

} // namespace blackbox::ingest