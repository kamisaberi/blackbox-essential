# 5-Minute Quickstart: In-Kernel Packet Drop Test

This walkthrough guides you through compiling a minimal C++20 program that attaches `xdp_filter.o` to a local network interface, blocks a test IP address, and verifies packet drops directly in kernel space.

---

## 1. Minimal Test Code (`drop_test.cpp`)

```cpp
#include <blackbox/blackbox.hpp>
#include <iostream>
#include <arpa/inet.h>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: sudo " << argv[0] << " <interface> <ip_to_block>\n"
                  << "Example: sudo " << argv[0] << " lo 127.0.0.99\n";
        return 1;
    }

    std::string interface_name = argv[1];
    std::string ip_str = argv[2];

    // 1. Convert IP string to network byte order uint32_t
    struct in_addr addr;
    if (inet_pton(AF_INET, ip_str.c_str(), &addr) != 1) {
        std::cerr << "[-] Invalid IP address format: " << ip_str << "\n";
        return 1;
    }

    std::cout << "[*] Configuring In-Kernel XDP Filter on interface: " << interface_name << "...\n";

    // 2. Initialize XDP Configuration
    blackbox::XdpConfig config{
        .interface_name = interface_name,
        .bpf_object_path = "/usr/local/lib/bpf/xdp_filter.o",
        // Loopback and virtual testing require SKB / Generic mode; physical NICs use DRIVER mode
        .attach_mode = (interface_name == "lo") ? blackbox::XdpAttachMode::SKB_GENERIC 
                                                : blackbox::XdpAttachMode::DRIVER,
        .max_blocked_ips = 1024
    };

    try {
        blackbox::XdpManager xdp(config);
        xdp.attach();
        std::cout << "[+] XDP Filter successfully attached.\n";

        // 3. Insert target IP into kernel blocked_ip_map with 30-second TTL
        uint64_t ttl_seconds = 30;
        xdp.block_ip(addr.s_addr, ttl_seconds);

        std::cout << "[!] IP " << ip_str << " BLOCKED in kernel space for " << ttl_seconds << "s.\n"
                  << "[*] Send packets now (e.g., ping -c 3 -I " << ip_str << " ...) to test drops.\n"
                  << "[*] Monitoring telemetry for 15 seconds...\n";

        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            auto telemetry = xdp.get_telemetry();
            std::cout << "  -> Total Processed: " << telemetry.total_packets_processed
                      << " | Total Dropped: " << telemetry.total_packets_dropped << "\n";
        }

        xdp.detach();
        std::cout << "[+] XDP Filter detached cleanly. Test completed.\n";

    } catch (const blackbox::BlackboxException& ex) {
        std::cerr << "[-] Fatal Error: " << ex.what() 
                  << " (Code: " << static_cast<int>(ex.error_code()) << ")\n";
        return 1;
    }

    return 0;
}
```

---

## 2. Compilation

```bash
clang++-16 -std=c++20 drop_test.cpp -o drop_test \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lblackbox \
    -Wl,-rpath,/usr/local/lib
```

---

## 3. Running the Test on Loopback

Execute the drop test on the local interface (requires root privileges to attach eBPF programs):

```bash
# Terminal 1: Run the drop test monitoring 127.0.0.99
sudo ./drop_test lo 127.0.0.99
```

In a second terminal, send ping packets bound to that address:

```bash
# Terminal 2: Test ICMP packet generation
ping -c 5 -I 127.0.0.99 127.0.0.1
```

### Expected Output in Terminal 1

```text
[*] Configuring In-Kernel XDP Filter on interface: lo...
[+] XDP Filter successfully attached.
[!] IP 127.0.0.99 BLOCKED in kernel space for 30s.
[*] Send packets now to test drops.
[*] Monitoring telemetry for 15 seconds...
  -> Total Processed: 5 | Total Dropped: 5
  -> Total Processed: 5 | Total Dropped: 5
[+] XDP Filter detached cleanly. Test completed.
```

In Terminal 2, you will observe 100% packet loss: `5 packets transmitted, 0 received, 100% packet loss`.

