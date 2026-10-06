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

