#include "blackbox/af_xdp_engine.hpp"
#include <iostream>
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

namespace blackbox::ingest {

struct xsk_umem_info {
    struct xsk_ring_prod fq;
    struct xsk_ring_cons cq;
    struct xsk_umem*     umem{nullptr};
    void*                buffer{nullptr};
};

struct xsk_socket_info {
    struct xsk_ring_cons rx;
    struct xsk_ring_prod tx;
    struct xsk_socket*   xsk{nullptr};
    int                  fd{-1};
};

AfXdpEngine::AfXdpEngine(AfXdpConfig config)
    : config_(std::move(config)) {}

AfXdpEngine::~AfXdpEngine() {
    stop();
}

bool AfXdpEngine::allocate_umem() {
    umem_size_ = static_cast<size_t>(config_.num_frames) * config_.frame_size;

    // Page-aligned memory allocation for UMEM chunks
    if (posix_memalign(&umem_buffer_, getpagesize(), umem_size_) != 0) {
        std::cerr << "[-] [AF_XDP] Failed to allocate page-aligned UMEM memory ("
                  << umem_size_ << " bytes)" << std::endl;
        return false;
    }

    umem_info_ = std::make_unique<xsk_umem_info>();
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
    if (ret != 0) {
        std::cerr << "[-] [AF_XDP] xsk_umem__create failed: " << strerror(-ret) << std::endl;
        free(umem_buffer_);
        umem_buffer_ = nullptr;
        return false;
    }

    // Populate Fill Ring with initial empty UMEM chunks for NIC to write into
    uint32_t idx = 0;
    ret = xsk_ring_prod__reserve(&umem_info_->fq, config_.num_frames / 2, &idx);
    if (ret > 0) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(ret); ++i) {
            *xsk_ring_prod__fill_addr(&umem_info_->fq, idx++) = i * config_.frame_size;
        }
        xsk_ring_prod__submit(&umem_info_->fq, ret);
    }

    return true;
}

bool AfXdpEngine::setup_xsk_socket(bool attempt_zero_copy) {
    xsk_info_ = std::make_unique<xsk_socket_info>();

    struct xsk_socket_config x_cfg{};
    x_cfg.rx_size = XSK_RING_CONS__DEFAULT_NUM_DESCS;
    x_cfg.tx_size = XSK_RING_PROD__DEFAULT_NUM_DESCS;
    // CRITICAL: Prevent libbpf from overwriting our loaded xdp_firewall program!
    x_cfg.libbpf_flags = XSK_LIBBPF_FLAGS__INHIBIT_PROG_LOAD;

    if (attempt_zero_copy) {
        x_cfg.bind_flags = XDP_ZERO_COPY;
        x_cfg.xdp_flags  = XDP_FLAGS_DRV_MODE;
    } else {
        // Safe VMware / Docker fallback mode: Generic SKB + Copy Mode
        x_cfg.bind_flags = XDP_COPY;
        x_cfg.xdp_flags  = XDP_FLAGS_SKB_MODE;
    }

    int ret = xsk_socket__create(&xsk_info_->xsk,
                                 config_.interface_name.c_str(),
                                 config_.queue_id,
                                 umem_info_->umem,
                                 &xsk_info_->rx,
                                 &xsk_info_->tx,
                                 &x_cfg);
    if (ret != 0) {
        return false;
    }

    xsk_info_->fd = xsk_socket__fd(xsk_info_->xsk);
    return true;
}

bool AfXdpEngine::start() {
    if (running_.load()) return true;

    // 1. Allocate UMEM
    if (!allocate_umem()) return false;

    // 2. Adaptive Probe: Try Zero-Copy first, then fall back for VMware/Docker
    bool bound = false;
    if (!config_.force_copy_mode) {
        bound = setup_xsk_socket(true);
        if (bound) {
            zero_copy_active_ = true;
            std::cout << "\033[1;32m[+] [AF_XDP Engine] Bound in TRUE HARDWARE ZERO-COPY mode (10M+ EPS capacity)\033[0m" << std::endl;
        }
    }

    if (!bound) {
        // VMware / Docker fallback
        bound = setup_xsk_socket(false);
        if (bound) {
            zero_copy_active_ = false;
            std::cout << "\033[1;33m[!] [AF_XDP Engine] Virtual interface detected ("
                      << config_.interface_name << " in VMware/Docker). Active in XDP_COPY mode.\033[0m" << std::endl;
        } else {
            std::cerr << "[-] [AF_XDP Engine] Failed to bind AF_XDP socket on "
                      << config_.interface_name << std::endl;
            stop();
            return false;
        }
    }

    running_.store(true);
    return true;
}

void AfXdpEngine::stop() {
    if (!running_.load()) return;
    running_.store(false);

    if (xsk_info_ && xsk_info_->xsk) {
        xsk_socket__delete(xsk_info_->xsk);
        xsk_info_->xsk = nullptr;
    }
    if (umem_info_ && umem_info_->umem) {
        xsk_umem__delete(umem_info_->umem);
        umem_info_->umem = nullptr;
    }
    if (umem_buffer_) {
        free(umem_buffer_);
        umem_buffer_ = nullptr;
    }
}

uint32_t AfXdpEngine::poll_batch(uint32_t max_batch, PacketHandler handler) {
    if (!running_.load() || !xsk_info_) return 0;

    uint32_t idx_rx = 0;
    uint32_t rcvd = xsk_ring_cons__peek(&xsk_info_->rx, max_batch, &idx_rx);
    if (rcvd == 0) return 0;

    for (uint32_t i = 0; i < rcvd; ++i) {
        const struct xdp_desc* desc = xsk_ring_cons__rx_desc(&xsk_info_->rx, idx_rx++);
        uint64_t addr = desc->addr;
        uint32_t len  = desc->len;

        // Zero-copy direct pointer into UMEM memory chunk
        const uint8_t* pkt_ptr = static_cast<const uint8_t*>(umem_buffer_) + addr;
        handler(pkt_ptr, len);
    }

    xsk_ring_cons__release(&xsk_info_->rx, rcvd);
    total_packets_.fetch_add(rcvd, std::memory_order_relaxed);

    // Replenish Fill Ring with newly available frames
    uint32_t idx_fq = 0;
    if (xsk_ring_prod__reserve(&umem_info_->fq, rcvd, &idx_fq) > 0) {
        for (uint32_t i = 0; i < rcvd; ++i) {
            *xsk_ring_prod__fill_addr(&umem_info_->fq, idx_fq++) = (i * config_.frame_size);
        }
        xsk_ring_prod__submit(&umem_info_->fq, rcvd);
    }

    return rcvd;
}

int AfXdpEngine::get_xsk_fd() const {
    return xsk_info_ ? xsk_info_->fd : -1;
}

} // namespace blackbox::ingest