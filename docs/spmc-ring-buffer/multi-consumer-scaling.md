---

### File: `blackbox-essential/docs/spmc-ring-buffer/multi-consumer-scaling.md`

```markdown
# Multi-Consumer Scaling & Contention Arbitration

While only one producer writes to the ring, multiple worker threads running AI inference models (`xinfer-essential`) dequeue events simultaneously. 

Consumers arbitrate slot access using an atomic Compare-And-Swap (CAS) loop on `read_index_`.

---

## 1. Consumer CAS Arbitration Loop

```text
 Consumer Thread A                                Consumer Thread B
 ┌───────────────────────────────────────┐        ┌───────────────────────────────────────┐
 │ 1. Read current head:                 │        │ 1. Read current head:                 │
 │    head = read_index_.load(relaxed)   │        │    head = read_index_.load(relaxed)   │
 │ 2. Check if data available (head < w) │        │ 2. Check if data available (head < w) │
 │ 3. Attempt CAS:                       │        │ 3. Attempt CAS:                       │
 │    CAS(read_index_, head, head + 1)   │        │    CAS(read_index_, head, head + 1)   │
 └──────────────────┬────────────────────┘        └──────────────────┬────────────────────┘
                    │                                                │
         SUCCESS ───┴───► [ Claims Slot N ]               FAILS ─────┴──► [ Retries Loop ]
```

---

## 2. Implementation: `try_dequeue()`

```cpp
bool EventRingBuffer::try_dequeue(FlowEvent& out_event) noexcept {
    uint64_t head = read_index_.load(std::memory_order_relaxed);

    while (true) {
        const uint64_t tail = write_index_.load(std::memory_order_acquire);

        // Ring is empty
        if (head >= tail) {
            return false;
        }

        // Attempt to atomically claim this slot
        if (read_index_.compare_exchange_weak(
                head, 
                head + 1, 
                std::memory_order_acquire, 
                std::memory_order_relaxed)) {
            
            // Successfully claimed: copy payload out
            out_event = slots_[head & index_mask_].payload;
            return true;
        }
        // CAS failed: head is updated automatically by compare_exchange_weak; retry
    }
}
```

---

## 3. Resolving the ABA Problem

Traditional lock-free queues that store pointers suffer from the **ABA problem**, where an address is freed, reallocated, and misidentified as unchanged.

`blackbox::EventRingBuffer` avoids this problem by design:
* It uses **monotonic 64-bit sequence counters** (`uint64_t`).
* At a rate of $10{,}000{,}000\text{ EPS}$, a 64-bit counter will not overflow for over **$58{,}494\text{ years}$**. Sequence counters never repeat within the operating lifetime of the appliance.
```

---

### File: `blackbox-essential/docs/spmc-ring-buffer/saturation-and-backpressure.md`

```markdown
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
```

---

### Complete in Part 4
- `blackbox-essential/docs/spmc-ring-buffer/lock-free-architecture.md`
- `blackbox-essential/docs/spmc-ring-buffer/atomic-memory-ordering.md`
- `blackbox-essential/docs/spmc-ring-buffer/cache-line-padding.md`
- `blackbox-essential/docs/spmc-ring-buffer/zero-mutex-producer.md`
- `blackbox-essential/docs/spmc-ring-buffer/multi-consumer-scaling.md`
- `blackbox-essential/docs/spmc-ring-buffer/saturation-and-backpressure.md`

All 6 lock-free SPMC ring buffer documentation files are now generated.

---

### Files to be Generated in Part 5

The next phase covers **Hardware Identity & Cryptographic TPM Attestation** (`hardware-identity-tpm/`):

1. `hardware-identity-tpm/attestation-architecture.md` (Why software-only identities fail in hostile environments)
2. `hardware-identity-tpm/tier1-physical-tpm2.md` (Interfacing with `/dev/tpmrm0` via TCG TSS2 specifications)
3. `hardware-identity-tpm/tpm2-pcr-measurements.md` (Generating cryptographic quotes over PCR 0 and PCR 4)
4. `hardware-identity-tpm/tier2-virtual-tpm.md` (Hypervisor attestation: VMware vTPM & QEMU swtpm)
5. `hardware-identity-tpm/tier3-dmi-fallback.md` (Motherboard DMI UUID hashing & fallback identities)
6. `hardware-identity-tpm/anti-cloning-protections.md` (Automated detection and revocation of cloned appliances)
7. `hardware-identity-tpm/quote-verification-flow.md` (Attestation Identity Key signature validation protocol)

Confirm when you are ready to proceed with Part 5.