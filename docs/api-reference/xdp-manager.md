# Class `blackbox::XdpManager`

Defined in header `<blackbox/xdp_manager.hpp>`  
Namespace: `blackbox`

`XdpManager` controls the in-kernel eBPF data plane. It compiles and loads BPF objects, hooks programs to network interfaces, mutates in-kernel hash tables (`blocked_ip_map`), and extracts per-CPU telemetry.

---

## 1. Class Synopsis

```cpp
namespace blackbox {

class BLACKBOX_API XdpManager {
public:
    explicit XdpManager(const XdpConfig& config);
    ~XdpManager();

    // Non-copyable, movable
    XdpManager(const XdpManager&) = delete;
    XdpManager& operator=(const XdpManager&) = delete;
    XdpManager(XdpManager&&) noexcept;
    XdpManager& operator=(XdpManager&&) noexcept;

    // Kernel Attachment & Lifecycle
    void attach();
    void detach() noexcept;
    [[nodiscard]] bool is_attached() const noexcept;

    // Active In-Kernel Mitigation
    void block_ip(uint32_t ipv4_net_order, uint64_t ttl_seconds, uint32_t rule_id = 0);
    void unblock_ip(uint32_t ipv4_net_order);
    [[nodiscard]] bool is_ip_blocked(uint32_t ipv4_net_order) const;

    // Telemetry & Introspection
    [[nodiscard]] KernelTelemetry get_telemetry() const;
    [[nodiscard]] std::vector<BlockedIpEntry> dump_blocked_ips() const;
    void flush_expired_entries();

    // Low-Level Descriptor Access
    [[nodiscard]] int get_bpf_program_fd() const noexcept;
    [[nodiscard]] int get_blocked_map_fd() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace blackbox
```

---

## 2. Member Functions

### `attach`
```cpp
void attach();
```
Validates kernel BTF support, loads the BPF object file into the kernel verifier, pins shared maps in `/sys/fs/bpf/blackbox`, and attaches `xdp_threat_filter()` to the configured interface. Throws `BlackboxException` if the driver hook fails or privileges are insufficient.

---

### `detach`
```cpp
void detach() noexcept;
```
Unhooks the XDP program from the network device driver and unpins transient maps. Executes automatically on destruction.

---

### `block_ip`
```cpp
void block_ip(uint32_t ipv4_net_order, uint64_t ttl_seconds, uint32_t rule_id = 0);
```
Inserts or updates an entry in the kernel's `blocked_ip_map`. `ipv4_net_order` must be in network byte order. Packets from this source address are dropped in $< 0.84\,\mu\text{s}$ until `ttl_seconds` elapses.

---

### `get_telemetry`
```cpp
KernelTelemetry get_telemetry() const;
```
Gathers per-CPU telemetry counters from the kernel and aggregates processed packets, bytes, and drop statistics.

