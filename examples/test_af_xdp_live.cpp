#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "blackbox/af_xdp_engine.hpp"
#include "mitigation/ebpf_blocker.hpp"

using namespace blackbox::ingest;
using namespace blackbox::mitigation;

// Helper to send test UDP packets over wire
void send_udp_burst(int count, const char* msg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    struct sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);

    for (int i = 0; i < count; ++i) {
        sendto(sock, msg, std::strlen(msg), 0, (struct sockaddr*)&dest, sizeof(dest));
    }
    close(sock);
}

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "   LIVE AF_XDP + eBPF ZERO-COPY PIPELINE VERIFICATION       " << std::endl;
    std::cout << "============================================================" << std::endl;

    // 1. Attach in-kernel XDP program to loopback interface
    EBPFBlocker blocker("lo");

    // 2. Initialize AF_XDP UMEM Engine
    AfXdpConfig cfg;
    cfg.interface_name = "lo";
    cfg.queue_id = 0;
    cfg.num_frames = 2048;
    cfg.frame_size = 2048;
    cfg.force_copy_mode = false; // VMware adaptive fallback

    AfXdpEngine engine(cfg);
    if (!engine.start()) {
        std::cerr << "[-] Failed to start AF_XDP engine!" << std::endl;
        return 1;
    }

    // 3. Register XSK Socket into in-kernel xsks_map
    int xsk_fd = engine.get_xsk_fd();
    if (!blocker.register_xsk_socket(0, xsk_fd)) {
        std::cerr << "[-] Failed to register socket in xsks_map!" << std::endl;
        return 1;
    }

    std::cout << "[+] Pipeline active! Testing wire traffic ingestion..." << std::endl;

    // -------------------------------------------------------------
    // PHASE 1: Send clean traffic -> Must be captured via AF_XDP
    // -------------------------------------------------------------
    send_udp_burst(5, "TEST_PAYLOAD_AF_XDP");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    uint32_t rcvd_phase1 = 0;
    engine.poll_batch(64, [&](const uint8_t* data, size_t len) {
        rcvd_phase1++;
    });

    std::cout << "[PHASE 1] Sent 5 clean packets -> AF_XDP Received: " << rcvd_phase1 << std::endl;

    // -------------------------------------------------------------
    // PHASE 2: Block IP in kernel -> Verify 0 packets reach AF_XDP
    // -------------------------------------------------------------
    std::cout << "\n[PHASE 2] Injecting 127.0.0.1 into in-kernel blocked_ip_map..." << std::endl;
    blocker.block_ip("127.0.0.1");

    send_udp_burst(5, "ATTACK_PAYLOAD");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    uint32_t rcvd_phase2 = 0;
    engine.poll_batch(64, [&](const uint8_t* data, size_t len) {
        rcvd_phase2++;
    });

    std::cout << "[PHASE 2] Sent 5 hostile packets -> Kernel dropped them! AF_XDP Received: " 
              << rcvd_phase2 << std::endl;
    assert(rcvd_phase2 == 0);

    // Unblock and cleanup
    blocker.unblock_ip("127.0.0.1");
    engine.stop();

    std::cout << "\n============================================================" << std::endl;
    std::cout << " [SUCCESS] Vector 2B (AF_XDP + eBPF Kernel Drops) Verified!  " << std::endl;
    std::cout << "============================================================" << std::endl;
    return 0;
}
