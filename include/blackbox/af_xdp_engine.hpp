#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <atomic>
#include <functional>
#include <memory>

// Forward declarations for opaque XSK structures
struct xsk_umem_info;
struct xsk_socket_info;
struct xsk_ring_cons;
struct xsk_ring_prod;

namespace blackbox::ingest {

struct AfXdpConfig {
    std::string interface_name{"eth0"};
    uint32_t    queue_id{0};
    uint32_t    num_frames{4096};
    uint32_t    frame_size{2048};
    bool        force_copy_mode{false}; // True for VMware/Docker virtual interfaces
};

// Callback invoked per packet without heap allocation: (packet_data, packet_len)
using PacketHandler = std::function<void(const uint8_t* data, size_t len)>;

class AfXdpEngine {
public:
    explicit AfXdpEngine(AfXdpConfig config);
    ~AfXdpEngine();

    AfXdpEngine(const AfXdpEngine&) = delete;
    AfXdpEngine& operator=(const AfXdpEngine&) = delete;

    /// Initialize UMEM memory pool, configure rings, and bind AF_XDP socket
    bool start();

    /// Stop rings and release UMEM
    void stop();

    /// Poll and process a batch of frames from Rx ring (called in high-speed loop)
    uint32_t poll_batch(uint32_t max_batch, PacketHandler handler);

    [[nodiscard]] bool is_zero_copy_active() const { return zero_copy_active_; }
    [[nodiscard]] bool is_running() const { return running_.load(); }
    [[nodiscard]] uint64_t total_packets_received() const { return total_packets_.load(); }
    [[nodiscard]] int get_xsk_fd() const;

private:
    bool allocate_umem();
    bool setup_xsk_socket(bool attempt_zero_copy);

    AfXdpConfig config_;
    std::atomic<bool> running_{false};
    bool zero_copy_active_{false};

    // UMEM & Rings internal state
    void*  umem_buffer_{nullptr};
    size_t umem_size_{0};

    std::unique_ptr<xsk_umem_info>   umem_info_;
    std::unique_ptr<xsk_socket_info> xsk_info_;

    std::atomic<uint64_t> total_packets_{0};
};

} // namespace blackbox::ingest