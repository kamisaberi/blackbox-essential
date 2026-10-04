---

### File: `blackbox-essential/docs/api-reference/event-ring-buffer.md`

```markdown
# Class `blackbox::EventRingBuffer`

Defined in header `<blackbox/event_ring_buffer.hpp>`  
Namespace: `blackbox`

`EventRingBuffer` is a lock-free Single-Producer Multi-Consumer (SPMC) circular queue designed to stream network flow telemetry from driver ingestion threads to parallel worker pools without mutex locks.

---

## 1. Class Synopsis

```cpp
namespace blackbox {

class BLACKBOX_API EventRingBuffer {
public:
    explicit EventRingBuffer(size_t capacity = 65536);
    ~EventRingBuffer();

    // Non-copyable, non-movable
    EventRingBuffer(const EventRingBuffer&) = delete;
    EventRingBuffer& operator=(const EventRingBuffer&) = delete;
    EventRingBuffer(EventRingBuffer&&) = delete;
    EventRingBuffer& operator=(EventRingBuffer&&) = delete;

    // Single-Producer Interface (Wait-Free)
    bool try_enqueue(const FlowEvent& event) noexcept;

    // Multi-Consumer Interface (Lock-Free CAS)
    bool try_dequeue(FlowEvent& out_event) noexcept;

    // Introspection & Capacity
    [[nodiscard]] size_t capacity() const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] uint64_t dropped_count() const noexcept;
    [[nodiscard]] RingBufferMetrics get_metrics() const noexcept;

    // Lifecycle
    void reset() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace blackbox
```

---

## 2. Producer & Consumer Guarantees

### `try_enqueue`
```cpp
bool try_enqueue(const FlowEvent& event) noexcept;
```
Called exclusively by the single network driver ingestion thread. Evaluates in $O(1)$ time with zero system calls. If the ring is saturated, increments `dropped_count_` and returns `false` (Tail Drop).

---

### `try_dequeue`
```cpp
bool try_dequeue(FlowEvent& out_event) noexcept;
```
Safe for concurrent invocation by multiple worker threads. Uses an atomic Compare-And-Swap (CAS) loop on the read index. Returns `true` if an event was claimed and copied into `out_event`, or `false` if the ring is empty.
```

---

### File: `blackbox-essential/docs/api-reference/hardware-identity.md`

```markdown
# Class `blackbox::HardwareIdentity`

Defined in header `<blackbox/hardware_identity.hpp>`  
Namespace: `blackbox`

`HardwareIdentity` provides cryptographic device attestation, managing interactions with physical TPM 2.0 chips, hypervisor vTPMs, and DMI fallback identifiers.

---

## 1. Class Synopsis

```cpp
namespace blackbox {

class BLACKBOX_API HardwareIdentity {
public:
    static HardwareIdentity& instance() noexcept;

    // Identity Discovery
    [[nodiscard]] IdentityTier active_tier() const noexcept;
    [[nodiscard]] std::string get_unique_identifier();
    [[nodiscard]] std::string manufacturer_id() const;
    [[nodiscard]] bool is_hardware_rooted() const noexcept;

    // Cryptographic Quote Generation
    [[nodiscard]] IdentityClaims generate_claims(std::span<const uint8_t, 32> nonce);
    [[nodiscard]] bool verify_claims(
        const IdentityClaims& claims, 
        std::span<const uint8_t, 32> nonce
    ) const;

    // Emergency Revocation
    void handle_revocation() noexcept;

private:
    HardwareIdentity();
    ~HardwareIdentity();
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace blackbox
```

---

## 2. Key Member Functions

### `generate_claims`
```cpp
IdentityClaims generate_claims(std::span<const uint8_t, 32> nonce);
```
Generates a signed TPM 2.0 quote certifying PCR 0 (BIOS) and PCR 4 (Bootloader) bound to the provided single-use 32-byte `nonce`. Returns an `IdentityClaims` structure containing the digital signature, PCR digest, and public certificates.

---

### `active_tier`
```cpp
IdentityTier active_tier() const noexcept;
```
Returns the operational identity level: `TIER1_PHYSICAL_TPM`, `TIER2_VTPM`, or `TIER3_DMI_FALLBACK`.
```

---

### File: `blackbox-essential/docs/api-reference/model-config.md`

```markdown
# Class `blackbox::ModelConfig`

Defined in header `<blackbox/model_config.hpp>`  
Namespace: `blackbox`

`ModelConfig` parses declarative YAML model manifests and coordinates dynamic tensor bindings between incoming network frames and neural network input layers.

---

## 1. Class Synopsis

```cpp
namespace blackbox {

class BLACKBOX_API ModelConfig {
public:
    static ModelConfig load_from_file(const std::filesystem::path& path);
    static ModelConfig parse_yaml(std::string_view yaml_content);

    ~ModelConfig();

    // Specification Accessors
    [[nodiscard]] const ModelMetadata& metadata() const noexcept;
    [[nodiscard]] const TensorInputSpec& input_spec() const noexcept;
    [[nodiscard]] const TensorOutputSpec& output_spec() const noexcept;
    [[nodiscard]] const MitigationPolicyConfig& policy() const noexcept;

    // Validation
    [[nodiscard]] bool validate() const;

private:
    ModelConfig();
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace blackbox
```

---

## 2. Usage Example

```cpp
#include <blackbox/model_config.hpp>
#include <iostream>

void load_and_inspect() {
    auto config = blackbox::ModelConfig::load_from_file("/etc/blackbox/model_config.yaml");

    std::cout << "Model Name   : " << config.metadata().model_name << "\n"
              << "Input Shape  : [" << config.input_spec().dimensions[0] << ", " 
                                    << config.input_spec().dimensions[1] << "]\n"
              << "Drop Action  : " << config.policy().on_anomaly << "\n";
}
```
```

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