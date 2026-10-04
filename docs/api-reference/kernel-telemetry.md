---

### File: `blackbox-essential/docs/api-reference/kernel-telemetry.md`

```markdown
# Struct `blackbox::KernelTelemetry`

Defined in header `<blackbox/telemetry.hpp>`  
Namespace: `blackbox`

`KernelTelemetry` encapsulates operational metrics gathered directly from the eBPF in-kernel data plane and network driver rings.

---

## 1. Structure Definition

```cpp
namespace blackbox {

struct KernelTelemetry {
    // Cumulative Packet Counters
    uint64_t total_packets_processed{0};
    uint64_t total_bytes_processed{0};
    uint64_t total_packets_dropped{0};
    uint64_t total_packets_passed{0};
    uint64_t parse_error_packets{0};

    // Derived Rate Metrics
    double current_pps{0.0};              // Packets per second
    double current_mbps{0.0};             // Megabits per second
    double drop_rate_percentage{0.0};     // Dropped / Total * 100.0

    // Table State
    uint64_t active_blocked_ips{0};       // Total entries in blocked_ip_map
    uint64_t expired_unreclaimed_ips{0};  // Entries pending garbage collection

    // Helper Methods
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] std::string to_json() const;
};

} // namespace blackbox
```

---

## 2. Field Descriptions

| Field | Type | Description |
| :--- | :--- | :--- |
| `total_packets_processed` | `uint64_t` | Total frames evaluated by `xdp_threat_filter()`. |
| `total_packets_dropped` | `uint64_t` | Total frames discarded at driver level via `XDP_DROP`. |
| `total_packets_passed` | `uint64_t` | Total clean frames forwarded to Linux network stack. |
| `drop_rate_percentage` | `double` | Current ratio of dropped traffic: $\frac{\text{dropped}}{\text{total}} \times 100$. |
| `active_blocked_ips` | `uint64_t` | Number of IPv4 addresses currently locked in kernel hash buckets. |
```

---

### File: `blackbox-essential/docs/api-reference/data-structures.md`

```markdown
# Core Data Structures & Configurations

Defined in header `<blackbox/types.hpp>`  
Namespace: `blackbox`

This document outlines the core data structures, configuration structs, and enumerations used across `blackbox-essential`.

---

## 1. Configuration Structures

### `XdpConfig`
Configuration parameters passed to `XdpManager`:
```cpp
struct XdpConfig {
    std::string interface_name{};
    std::filesystem::path bpf_object_path{"/usr/local/lib/bpf/xdp_filter.o"};
    XdpAttachMode attach_mode{XdpAttachMode::DRIVER};
    size_t max_blocked_ips{65536};
    bool enable_promiscuous{false};
    uint32_t xsk_queue_count{1};
};
```

### `XdpAttachMode`
Specifies how the eBPF bytecode hooks into the network device:
```cpp
enum class XdpAttachMode : uint32_t {
    DRIVER = 0,         // Native driver space: Sub-microsecond (< 0.84µs) SLA
    SKB_GENERIC,        // Generic fallback mode: Operates after sk_buff allocation
    HARDWARE_OFFLOAD    // SmartNIC offload directly inside network processor ASIC
};
```

---

## 2. Operational Structures

### `BlockedIpEntry`
Represents an active in-kernel mitigation rule:
```cpp
struct BlockedIpEntry {
    uint32_t ipv4_address;          // Network byte order
    uint64_t expire_timestamp_ns;   // CLOCK_MONOTONIC nanosecond expiry
    uint64_t drop_count;            // Drops executed against this IP
    uint32_t rule_id;               // Triggering security policy ID
    uint32_t flags;                 // Status and escalation bitmask
};
```

### `FlowEvent`
Telemetry event structure passed across the lock-free ring buffer:
```cpp
struct alignas(64) FlowEvent {
    uint64_t timestamp_ns{0};
    uint32_t src_ip{0};
    uint32_t dst_ip{0};
    uint16_t src_port{0};
    uint16_t dst_port{0};
    uint16_t packet_length{0};
    uint8_t protocol{0};
    uint8_t tcp_flags{0};
    uint8_t payload_preview[32]{};
};
```

### `IdentityClaims`
Cryptographic bundle produced during hardware attestation:
```cpp
struct IdentityClaims {
    IdentityTier tier{IdentityTier::TIER3_DMI_FALLBACK};
    std::string node_id{};
    uint32_t pcr_mask{0};
    std::vector<uint8_t> pcr_digest{};
    std::vector<uint8_t> quote_signature{};
    std::vector<uint8_t> aik_public_cert{};
    uint64_t timestamp_ns{0};
};
```
```

---

### File: `blackbox-essential/docs/api-reference/error-codes.md`

```markdown
# Class `blackbox::BlackboxException` & Error Codes

Defined in header `<blackbox/exception.hpp>`  
Namespace: `blackbox`

`blackbox-essential` handles control-plane failures and operational errors using strongly-typed exceptions derived from `std::exception`.

---

## 1. Class Synopsis

```cpp
namespace blackbox {

enum class ErrorCode : int32_t {
    SUCCESS = 0,
    ERR_XDP_ATTACH_FAILED = -1,
    ERR_XDP_DETACH_FAILED = -2,
    ERR_BPF_OBJECT_NOT_FOUND = -3,
    ERR_BPF_MAP_NOT_FOUND = -4,
    ERR_BPF_MAP_UPDATE_FAILED = -5,
    ERR_BPF_MAP_LOOKUP_FAILED = -6,
    ERR_INTERFACE_NOT_FOUND = -7,
    ERR_TPM_INITIALIZATION_FAILED = -8,
    ERR_TPM_QUOTE_FAILED = -9,
    ERR_MODEL_CONFIG_INVALID = -10,
    ERR_RING_BUFFER_FULL = -11,
    ERR_PERMISSION_DENIED = -12,
    ERR_INTERNAL_FAULT = -99
};

class BLACKBOX_API BlackboxException : public std::exception {
public:
    explicit BlackboxException(ErrorCode code, std::string message, int system_code = 0) noexcept;
    ~BlackboxException() override = default;

    [[nodiscard]] const char* what() const noexcept override;
    [[nodiscard]] ErrorCode error_code() const noexcept;
    [[nodiscard]] int system_code() const noexcept;

private:
    ErrorCode code_;
    std::string message_;
    int system_code_{0};
};

} // namespace blackbox
```

---

## 2. Idiomatic Exception Handling

```cpp
#include <blackbox/blackbox.hpp>
#include <iostream>

int main() {
    try {
        blackbox::XdpConfig config{
            .interface_name = "eth0",
            .bpf_object_path = "/usr/local/lib/bpf/xdp_filter.o"
        };

        blackbox::XdpManager xdp(config);
        xdp.attach();

    } catch (const blackbox::BlackboxException& ex) {
        std::cerr << "[Blackbox Fatal Error]\n"
                  << "  Code       : " << static_cast<int>(ex.error_code()) << "\n"
                  << "  Description: " << ex.what() << "\n"
                  << "  System Err : " << ex.system_code() << " (" 
                  << std::strerror(ex.system_code()) << ")\n";
        return 1;
    }

    return 0;
}
```
```

---

### Complete in Part 8
- `blackbox-essential/docs/api-reference/index.md`
- `blackbox-essential/docs/api-reference/xdp-manager.md`
- `blackbox-essential/docs/api-reference/event-ring-buffer.md`
- `blackbox-essential/docs/api-reference/hardware-identity.md`
- `blackbox-essential/docs/api-reference/model-config.md`
- `blackbox-essential/docs/api-reference/kernel-telemetry.md`
- `blackbox-essential/docs/api-reference/data-structures.md`
- `blackbox-essential/docs/api-reference/error-codes.md`

All 8 API reference files for `blackbox-essential` are now generated.

---

### Files to be Generated in Part 9

The next phase covers **Step-by-Step Practical Tutorials** (`tutorials/`):

1. `tutorials/building-in-kernel-firewall.md` (Creating a wire-speed packet blocker in 50 lines of C++20)
2. `tutorials/attaching-xdp-to-vmware-vnic.md` (Configuring eBPF on VMware ens33 / vmxnet3 virtual interfaces)
3. `tutorials/extracting-tpm2-quotes.md` (Reading and verifying physical TPM 2.0 silicon quotes)
4. `tutorials/wiring-xdp-to-xinfer.md` (Connecting eBPF packet capture to xInfer neural scoring)
5. `tutorials/high-rate-packet-blaster-testing.md` (Stress-testing the SPMC ring buffer with 1M+ packets/sec)

Confirm when you are ready to proceed with Part 9.