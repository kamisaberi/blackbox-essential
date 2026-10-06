# Saturation, Backpressure & Tail-Drop Mechanics

When edge appliances experience volumetric distributed denial-of-service (DDoS) attacks, network ingestion rates can exceed downstream AI inference throughput. 

To maintain system stability, `blackbox::EventRingBuffer` implements deterministic **Tail-Drop backpressure mechanics**.

---

## 1. Backpressure Strategies Comparison

```text
 1. BLOCKING PRODUCER (Dangerous for Edge Defense)
    - Producer pauses until consumers process items.
    - Result: Driver packet ring overflows; kernel drops legitimate packets.

 2. HEAD-DROP (Oldest Eviction)
    - Producer overwrites oldest unprocessed events.
    - Result: Requires updating consumer pointers; introduces race conditions.

 3. TAIL-DROP (Deterministic Edge Invariant - Used by Blackbox)
    - When ring is full, producer drops the newest inbound telemetry event.
    - Result: Zero impact on consumer state; drops recorded for telemetry.
```

---

## 2. Saturation Monitoring & Adaptive Alerts

The control plane monitors ring buffer saturation and alerts user-space orchestrators if capacity limits are breached:

```cpp
struct RingBufferMetrics {
    uint64_t capacity;
    uint64_t occupied_slots;
    double saturation_percentage;
    uint64_t total_dropped_events;
};

RingBufferMetrics EventRingBuffer::get_metrics() const noexcept {
    const uint64_t tail = write_index_.load(std::memory_order_relaxed);
    const uint64_t head = read_index_.load(std::memory_order_relaxed);
    const uint64_t occupied = (tail > head) ? (tail - head) : 0;

    return RingBufferMetrics{
        .capacity = capacity_,
        .occupied_slots = occupied,
        .saturation_percentage = (static_cast<double>(occupied) / capacity_) * 100.0,
        .total_dropped_events = dropped_count_.load(std::memory_order_relaxed)
    };
}
```

---

## 3. Hardware Backpressure Signaling to eBPF

If saturation exceeds $95\%$, `libblackbox.so` signals the in-kernel eBPF filter (`xdp_filter.o`) to enable aggressive rate limiting:

```text
[ RingBuffer Saturation > 95% ]
               │
               ▼ Control Plane Updates BPF Flag
[ BPF Map: sys_config_map[RATE_LIMIT_MODE] = 1 ]
               │
               ▼ In-Kernel Fast Path
[ eBPF Driver Drops Ingress Packets via XDP_DROP at Wire Speed ]
```

This prevents un-evaluated packets from flooding user-space memory while keeping the host operating system responsive.
