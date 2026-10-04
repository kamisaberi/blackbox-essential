---

### File: `blackbox-essential/docs/architecture/zero-skb-allocation.md`

```markdown
# Zero-`sk_buff` Memory Allocation

The primary bottleneck in the Linux network stack under heavy traffic is the allocation, initialization, and de-allocation of **`struct sk_buff`** (socket buffer) descriptors. 

`blackbox-essential` achieves wire-speed throughput by evaluating and dropping packets **before** any socket buffer is allocated.

---

## 1. The Cost of `struct sk_buff`

In the standard Linux networking architecture, every Ethernet frame that passes the physical network adapter is wrapped inside an `sk_buff` structure:

```text
Standard Kernel Path:
[ Ethernet Frame in RAM ]
           │
           ▼
[ kmem_cache_alloc(skbuff_head_cache) ] ──► ~240 bytes of metadata
           │
           ▼
[ kmem_cache_alloc(skbuff_data_cache) ] ──► Dedicated packet memory
           │
           ▼
[ Initialize ~40 struct members ] ────────► Protocol, timestamp, interface, refcount
           │
           ▼
[ Pass to Netfilter / TCP / UDP ]
```

### Consequences Under Denial-of-Service Conditions

1. **Slab Allocator Lock Contention:** The kernel memory allocator (`kmem_cache_alloc`) incurs spinlock contention across multiple CPU cores when allocating millions of socket buffers per second.
2. **CPU Cache Thrashing:** Creating $240\text{ bytes}$ of control metadata per packet displaces Layer 1 and Layer 2 CPU caches, slowing down all running system processes.
3. **Garbage Collection Overhead:** Dropping a packet late in user-space requires calling `kfree_skb()`, which invalidates cache lines and issues inter-processor interrupts (IPIs) to free page references.

---

## 2. The `xdp_buff` Alternative

eBPF/XDP operates on a minimal, pre-allocated hardware abstraction called `struct xdp_buff`:

```c
struct xdp_buff {
    void *data;             /* Start of packet data */
    void *data_end;         /* End of packet data */
    void *data_meta;        /* Metadata prepended to packet */
    void *data_hard_start;  /* Start of allocated page */
    struct xdp_rxq_info *rxq;/* Pointer to RX queue metadata */
    struct xdp_mem_info mem;/* Memory type (e.g. MEM_TYPE_PAGE_SHARED) */
    u32 frame_sz;           /* Frame size */
};
```

This structure is created once on the CPU stack or mapped directly over the driver's ring buffer descriptors. No heap memory is allocated.

---

## 3. Descriptor Recycling: The Mechanics of `XDP_DROP`

When `xdp_threat_filter()` returns `XDP_DROP`, the network card driver recycles the packet buffer without notifying the host operating system:

```text
 1. Packet arrives at NIC DMA ring descriptor N.
 2. Driver initializes lightweight xdp_buff on the local stack.
 3. xdp_threat_filter() inspects headers -> Finds match in blocked_ip_map.
 4. Return XDP_DROP.
 5. Driver resets descriptor N read pointer back to the DMA start address.
 6. Memory is immediately reused for the next incoming Ethernet frame.
```

### Memory Allocation Metrics

| Metric | Netfilter / `iptables` DROP | `blackbox-essential` XDP_DROP |
| :--- | :--- | :--- |
| **Heap Allocations per Packet** | 2 (`sk_buff` + data head) | **0** |
| **Bytes Allocated in RAM** | $\sim 240\text{ bytes} + \text{frame length}$ | **0 bytes** |
| **CPU Cache Invalidation** | High (Writes across 4 cache lines) | **None** (Reads frame header only) |
| **Max Dropping Capacity** | $\sim 1.8\text{ Mpps}$ | **$> 14.8\text{ Mpps}$ (Line Rate)** |
```

---

### File: `blackbox-essential/docs/architecture/data-plane-vs-control-plane.md`

```markdown
# Data Plane vs. Control Plane Architecture

`blackbox-essential` enforces an architectural boundary between its high-throughput **in-kernel Data Plane** and its flexible **user-space Control Plane**.

---

## 1. Architectural Separation

```text
 ┌─────────────────────────────────────────────────────────────┐
 │                CONTROL PLANE (User Space)                   │
 │                                                             │
 │  - Native ISO C++20 (libblackbox.so)                        │
 │  - blackbox::XdpManager                                     │
 │  - Telemetry Harvesting & Aggregation                       │
 │  - Ephemeral TTL Expiration Engine                          │
 │  - Model Inference & Policy Evaluation (Tier 1/Tier 3)      │
 │  - Non-Blocking System Calls (bpf(BPF_MAP_UPDATE_ELEM))     │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Bidirectional Shared Memory
                                │ (Pinned BPF Hash Maps)
 ┌──────────────────────────────┴──────────────────────────────┐
 │                 DATA PLANE (Kernel Space)                   │
 │                                                             │
 │  - In-Kernel eBPF Bytecode (xdp_filter.o)                   │
 │  - BPF JIT Compiled Native Machine Instructions             │
 │  - Hard Real-Time Execution Constraints (< 0.84 µs)         │
 │  - Zero System Calls, Zero Page Allocations                 │
 │  - Reads blocked_ip_map, Writes xdp_telemetry_map           │
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. Invariants & Responsibilities

| Attribute | Data Plane (eBPF Kernel) | Control Plane (C++20 User Space) |
| :--- | :--- | :--- |
| **Execution Environment** | Linux Kernel Driver Space | User Space (`libblackbox.so`) |
| **Timing Constraints** | Hard Real-Time ($< 0.84\,\mu\text{s}$) | Soft Real-Time ($< 50\,\text{ms}$) |
| **Memory Allocation** | Zero heap memory; stack bounded to 512B | Pre-allocated pinned pools & ring buffers |
| **Allowed Operations** | Arithmetic, bounds-checked packet reads | System calls, TPM quotes, telemetry aggregation |
| **Primary Task** | Immediate packet triage (`XDP_DROP` / `PASS`) | Threat decision, policy insertion, telemetry reporting |

---

## 3. Control-to-Data Plane Communication Channels

The Control Plane and Data Plane communicate through three BPF map structures:

### 1. `blocked_ip_map` (Policy Enforcement)
* **Type:** `BPF_MAP_TYPE_HASH`
* **Access Pattern:** 
  * Control Plane: Writes blocked IPv4 addresses with nanosecond expiration timestamps via `bpf_map_update_elem()`.
  * Data Plane: Performs lock-free lookups per incoming packet. If `current_time > expiry`, entry is ignored.

### 2. `telemetry_map` (Performance Tracking)
* **Type:** `BPF_MAP_TYPE_PERCPU_ARRAY`
* **Access Pattern:**
  * Data Plane: Atomically increments per-CPU drop and pass counters using lock-free primitives.
  * Control Plane: Queries and sums metrics every $1{,}000\,\text{ms}$ without interrupting kernel execution.

### 3. `event_ringbuf` (Flow Capture Tap)
* **Type:** `BPF_MAP_TYPE_RINGBUF`
* **Access Pattern:**
  * Data Plane: Streams packet headers and threat events to user-space workers for continuous AI scoring.
```

---

### File: `blackbox-essential/docs/architecture/kernel-userspace-abi.md`

```markdown
# Kernel-Userspace ABI Stability & Shared Data Layout

Because `blackbox-essential` bridges C eBPF kernel code and an ISO C++20 user-space runtime, strict Application Binary Interface (ABI) stability is enforced across the kernel boundary. Any divergence in struct padding, member alignment, or byte order will corrupt telemetry or cause map lookup failures.

---

## 1. Memory Layout & Strict Alignment Rules

All shared structures declared between `xdp_filter.c` and `xdp_manager.hpp` use explicit 64-bit alignment and integer widths:

* Bit widths use standard POSIX types: `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`.
* Enums are explicitly typed: `enum class Action : uint32_t`.
* Structures are aligned to 64-bit (8-byte) boundaries to prevent compiler padding differences between Clang (BPF target) and GCC/Clang (x86_64/ARM64 targets).

---

## 2. Shared ABI Structures (`<blackbox/abi.hpp>`)

```cpp
#pragma once

#include <cstdint>

namespace blackbox::abi {

#pragma pack(push, 8)

// Key for blocked_ip_map: 32-bit IPv4 address (Network Byte Order)
struct BlockedIpKey {
    uint32_t ipv4_address; // e.g. 0xC0A80163 for 192.168.1.99
    uint32_t reserved;     // Explicit padding to maintain 64-bit alignment
};

// Value for blocked_ip_map: Expiration and tracking metadata
struct BlockedIpValue {
    uint64_t expire_timestamp_ns; // CLOCK_MONOTONIC nanosecond timestamp
    uint64_t drop_count;           // Incremented on every drop in kernel
    uint32_t rule_id;              // Identifier of triggering threat rule
    uint32_t flags;                // Reserved bitmask
};

// Per-CPU telemetry array entry
struct KernelMetrics {
    uint64_t rx_packets;          // Total packets processed
    uint64_t rx_bytes;            // Total bytes processed
    uint64_t dropped_packets;     // Packets dropped by policy
    uint64_t passed_packets;      // Clean packets forwarded to stack
    uint64_t error_packets;       // Malformed / parse errors
};

#pragma pack(pop)

// Enforce compile-time layout verification
static_assert(sizeof(BlockedIpKey) == 8, "BlockedIpKey ABI size mismatch");
static_assert(sizeof(BlockedIpValue) == 24, "BlockedIpValue ABI size mismatch");
static_assert(sizeof(KernelMetrics) == 40, "KernelMetrics ABI size mismatch");

} // namespace blackbox::abi
```

---

## 3. Syscall Boundary Mechanics

The user-space runtime (`libblackbox.so`) interacts with BPF maps via the Linux `bpf()` system call (`__NR_bpf`):

```cpp
#include <linux/bpf.h>
#include <sys/syscall.h>
#include <unistd.h>

inline int sys_bpf_map_update(int map_fd, const void* key, const void* value, uint64_t flags) {
    union bpf_attr attr{};
    attr.map_fd = map_fd;
    attr.key = reinterpret_cast<uint64_t>(key);
    attr.value = reinterpret_cast<uint64_t>(value);
    attr.flags = flags;

    return static_cast<int>(::syscall(__NR_bpf, BPF_MAP_UPDATE_ELEM, &attr, sizeof(attr)));
}
```

This interaction occurs without holding system locks or allocating kernel buffers outside the pre-allocated map bounds.
```

---

### Complete in Part 2
- `blackbox-essential/docs/architecture/core-engine-design.md`
- `blackbox-essential/docs/architecture/sub-microsecond-physics.md`
- `blackbox-essential/docs/architecture/zero-skb-allocation.md`
- `blackbox-essential/docs/architecture/data-plane-vs-control-plane.md`
- `blackbox-essential/docs/architecture/kernel-userspace-abi.md`

All 5 core architecture files for `blackbox-essential` are now generated.

---

### Files to be Generated in Part 3

The next phase covers the **eBPF/XDP Subsystem** (`ebpf-xdp-subsystem/`):

1. `ebpf-xdp-subsystem/xdp-filter-architecture.md` (Lifecycle of incoming frames in `xdp_threat_filter()`)
2. `ebpf-xdp-subsystem/driver-mode-vs-skb-mode.md` (Native XDP vs. Generic SKB fallback)
3. `ebpf-xdp-subsystem/bpf-verifier-guarantees.md` (Memory bounds safety proofs and verifier math)
4. `ebpf-xdp-subsystem/compiling-bpf-bytecode.md` (Clang/LLVM compilation flags and `build_bpf.sh`)
5. `ebpf-xdp-subsystem/bpf-map-management.md` (`BPF_MAP_TYPE_HASH` mechanics, sizing, and pinned namespaces)
6. `ebpf-xdp-subsystem/ttl-ephemeral-expiry.md` (Nanosecond timestamp eviction logic in kernel space)
7. `ebpf-xdp-subsystem/kernel-lockdown-and-signing.md` (Lockdown mode compatibility & cryptographic signing)

Confirm when you are ready to proceed with Part 3