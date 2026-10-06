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

