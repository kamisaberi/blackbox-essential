#include <iostream>
#include <chrono>
#include <thread>
#include "blackbox/af_xdp_engine.hpp"

using namespace blackbox::ingest;

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "   TESTING ADAPTIVE AF_XDP ENGINE (VMWARE / DOCKER SAFE)   " << std::endl;
    std::cout << "============================================================" << std::endl;

    AfXdpConfig cfg;
    cfg.interface_name = "lo"; // Loopback interface (standard Linux test)
    cfg.queue_id = 0;
    cfg.num_frames = 2048;
    cfg.frame_size = 2048;
    cfg.force_copy_mode = false; // Tests auto-fallback!

    AfXdpEngine engine(cfg);
    if (!engine.start()) {
        std::cerr << "[-] Failed to start AF_XDP engine on interface!" << std::endl;
        return 1;
    }

    std::cout << "[+] Engine Online! Mode: " 
              << (engine.is_zero_copy_active() ? "HARDWARE ZERO-COPY" : "VMWARE/GENERIC XDP_COPY") 
              << std::endl;

    // Simulate batch poll (0 packets on quiet interface)
    uint32_t packets = engine.poll_batch(64, [](const uint8_t* data, size_t len) {
        std::cout << "[PACKET] Received " << len << " bytes via AF_XDP UMEM" << std::endl;
    });

    std::cout << "[+] Poll executed cleanly. Packets polled: " << packets << std::endl;
    engine.stop();
    std::cout << "[SUCCESS] AF_XDP Engine initialized and torn down cleanly!" << std::endl;
    return 0;
}
