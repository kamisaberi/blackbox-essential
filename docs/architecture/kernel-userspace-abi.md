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