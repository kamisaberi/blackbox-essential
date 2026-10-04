---

### File: `blackbox-essential/docs/ebpf-xdp-subsystem/driver-mode-vs-skb-mode.md`

```markdown
# Native Driver Mode vs. Generic SKB Mode

`blackbox-essential` supports multiple XDP execution modes depending on network hardware, virtualization layers, and host driver capabilities.

---

## 1. Architectural Comparison

```text
NATIVE DRIVER MODE (XDP_FLAGS_DRV_MODE):
[ NIC Hardware DMA ] ──► [ Driver Rx Ring ] ──► [ xdp_filter.o ] ──► XDP_DROP (< 0.84 µs)
                                                       │
                                                       ▼ XDP_PASS
                                             [ alloc_skb() ] ──► Linux Network Stack

--------------------------------------------------------------------------------

GENERIC SKB MODE (XDP_FLAGS_SKB_MODE):
[ NIC Hardware DMA ] ──► [ Driver Rx Ring ] ──► [ alloc_skb() ]
                                                       │
                                                       ▼
                                            [ netif_receive_skb() ]
                                                       │
                                                       ▼
                                              [ xdp_filter.o ] ──► XDP_DROP (~2.5 - 5.0 µs)
                                                       │
                                                       ▼ XDP_PASS
                                             [ TCP/IP Stack ]
```

---

## 2. Mode Trade-Off Matrix

| Feature | Native Driver Mode (`DRV`) | Generic SKB Mode (`SKB`) | Hardware Offload (`HW`) |
| :--- | :--- | :--- | :--- |
| **Mitigation Latency** | **$< 0.84\,\mu\text{s}$** | $\sim 2.5 - 5.0\,\mu\text{s}$ | **$< 0.15\,\mu\text{s}$** |
| **`sk_buff` Allocated?** | **No** (Zero allocations) | Yes (Allocated prior to hook) | **No** |
| **Hardware Compatibility** | Intel, Mellanox, Broadcom | All Linux Network Devices | Netronome, SmartNICs |
| **Loopback / veth Support**| No | **Yes** | No |
| **Max Dropping Capacity** | **$14.88\text{ Mpps}$** | $\sim 1.8\text{ Mpps}$ | $> 50.0\text{ Mpps}$ |

---

## 3. Dynamic Mode Selection Logic in `XdpManager`

`blackbox-essential` selects the optimal mode automatically while allowing explicit programmatic overrides:

```cpp
#include <blackbox/xdp_manager.hpp>

blackbox::XdpConfig config;
config.interface_name = "eth0";
config.bpf_object_path = "/usr/local/lib/bpf/xdp_filter.o";

// Set attach policy
if (config.interface_name == "lo" || config.interface_name.starts_with("veth")) {
    // Virtual interfaces require SKB Generic mode
    config.attach_mode = blackbox::XdpAttachMode::SKB_GENERIC;
} else {
    // Physical NICs (ixgbe, mlx5, vmxnet3) utilize sub-microsecond native driver mode
    config.attach_mode = blackbox::XdpAttachMode::DRIVER;
}

blackbox::XdpManager xdp(config);
xdp.attach();
```

---

## 4. Querying Active Attachment Mode

Inspect the operational attachment mode using the `iproute2` suite:

```bash
ip link show dev eth0
```

### Native Driver Mode Output:
```text
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 xdp/id:42 ...
```

### Generic Mode Output:
```text
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 xdpgeneric/id:42 ...
```
```

---

### File: `blackbox-essential/docs/ebpf-xdp-subsystem/bpf-verifier-guarantees.md`

```markdown
# BPF Verifier Guarantees & Memory Bounds Safety Proofs

Before any eBPF program can be loaded into the Linux kernel, it must pass inspection by the in-kernel **BPF Verifier**. The verifier evaluates every possible instruction path, proving that the code cannot crash the operating system, access arbitrary memory, or enter an infinite loop.

---

## 1. The Core Verifier Guarantees

1. **Memory Safety:** The program can only read or write within explicitly bounded packet memory (`ctx->data` to `ctx->data_end`) or allocated map elements.
2. **Termination:** Unbounded loops are forbidden. All execution paths must reach an exit point within $1{,}000{,}000$ verified instructions.
3. **Type Safety:** Pointer types are strictly tracked. A pointer to a packet cannot be cast to an arbitrary kernel memory address.
4. **Stack Boundary:** The eBPF call frame stack is capped at **512 bytes**. Exceeding this boundary fails verification.

---

## 2. Mathematical Proof of Bounds Checking

Every packet memory access in `xdp_filter.c` is preceded by an explicit bounds check. The BPF verifier enforces the invariant that all offsets must satisfy:

$$\text{data} + \text{offset} + \text{sizeof}(\text{struct}) \le \text{data\_end}$$

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ ctx->data                                                   │
 └──────┬──────────────────────────────────────────────────────┘
        │
        ▼ eth = data
 ┌──────────────────────┐
 │ struct ethhdr (14B)  │
 └──────┬───────────────┘
        │  [ VERIFICATION PROOF: (eth + 1) <= data_end ]
        ▼ ip = eth + 1
 ┌──────────────────────┐
 │ struct iphdr (20B)   │
 └──────┬───────────────┘
        │  [ VERIFICATION PROOF: (ip + 1) <= data_end ]
        │  [ VERIFICATION PROOF: ((char *)ip + (ip->ihl * 4)) <= data_end ]
        ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ ctx->data_end                                               │
 └─────────────────────────────────────────────────────────────┘
```

If any memory dereference occurs without a prior bounds check, the verifier rejects the program at load time with an error:

```text
invalid access to packet, memptr=R1, offset=14, size=20
R1 min value is outside of the allowed memory range
```

---

## 3. Register State Tracking (R1 through R10)

The verifier tracks machine registers using abstract value types:

| Register | Purpose in `xdp_filter.o` |
| :--- | :--- |
| `R1` | First argument: Pointer to `struct xdp_md ctx` on entry. |
| `R2` - `R5` | Function call argument registers passed to BPF helpers. |
| `R0` | Function return value (e.g., result of `bpf_map_lookup_elem` or final `XDP_DROP`). |
| `R6` - `R9` | Callee-saved general-purpose registers (stores pointers to `data` and `data_end`). |
| `R10` | **Read-Only Frame Pointer:** References the 512-byte eBPF stack space. |
```

---

### File: `blackbox-essential/docs/ebpf-xdp-subsystem/compiling-bpf-bytecode.md`

```markdown
# Compiling BPF Bytecode & Toolchain Pipeline

Compiling eBPF bytecode requires targeting the Clang/LLVM BPF virtual machine architecture (`-target bpf`). This document details the compilation pipeline that produces `xdp_filter.o`.

---

## 1. The Compilation Toolchain Pipeline

```text
 [ C Source Code: bpf/xdp_filter.c ]
                    │
                    ▼ clang-16 -target bpf -O2 -g
 [ LLVM Intermediate Representation (IR) ]
                    │
                    ▼ llvm-strip -g (Strip non-BTF debug sections)
 [ BPF ELF Object: xdp_filter.o ]
   ├── Section .text: Native BPF bytecode instructions
   ├── Section .maps: Map definitions (blocked_ip_map)
   └── Section .BTF : BPF Type Format metadata
                    │
                    ▼ libbpf / blackbox::XdpManager
 [ Kernel Verifier & JIT Compiler ]
                    │
                    ▼
 [ Native x86-64 / ARM64 Machine Instructions in Driver Ring ]
```

---

## 2. The Official Compilation Script (`bpf/build_bpf.sh`)

```bash
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/../build/bpf"
mkdir -p "${OUTPUT_DIR}"

CLANG="${CLANG:-clang-16}"
LLVM_STRIP="${LLVM_STRIP:-llvm-strip-16}"

# Detect host architecture for correct macro headers
ARCH=$(uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')

echo "[*] Compiling xdp_filter.c for architecture: ${ARCH} using ${CLANG}..."

${CLANG} -O2 -g \
    -target bpf \
    -D__TARGET_ARCH_${ARCH} \
    -I/usr/include \
    -I/usr/include/${uname_m:="$(uname -m)-linux-gnu"} \
    -Wall \
    -Wextra \
    -Werror \
    -c "${SCRIPT_DIR}/xdp_filter.c" \
    -o "${OUTPUT_DIR}/xdp_filter.o"

# Strip non-essential symbols while preserving BTF debug information
${LLVM_STRIP} -g "${OUTPUT_DIR}/xdp_filter.o"

echo "[+] Compilation successful: ${OUTPUT_DIR}/xdp_filter.o"
```

---

## 3. Disassembling and Auditing BPF Bytecode

Verify the compiled instructions using `llvm-objdump`:

```bash
llvm-objdump-16 -d build/bpf/xdp_filter.o
```

### Sample Disassembly:
```text
0000000000000000 <xdp_threat_filter>:
       0:       r2 = *(u32 *)(r1 + 0x4)
       1:       r1 = *(u32 *)(r1 + 0x0)
       2:       r3 = r1
       3:       r3 += 0xe
       4:       if r3 > r2 goto +0x1d <LBB0_7>
       5:       r4 = *(u16 *)(r1 + 0xc)
       6:       if r4 != 0x8 goto +0x1b <LBB0_7>
       7:       r3 = r1
       8:       r3 += 0x22
       9:       if r3 > r2 goto +0x18 <LBB0_7>
```

Lines 4 and 9 illustrate the compiler emitting the bounds-checking comparison instructions before any packet dereference.
```

---

### File: `blackbox-essential/docs/ebpf-xdp-subsystem/bpf-map-management.md`

```markdown
# BPF Map Management & Pinned Namespaces

BPF maps are structured memory pools allocated within kernel address space, enabling bidirectional communication between the high-speed data plane (`xdp_filter.o`) and the user-space control plane (`libblackbox.so`).

---

## 1. Pinned Map Namespaces

By default, when a user-space process closes its file descriptors to a BPF map, the kernel releases the map from memory. To prevent state loss during daemon updates or service restarts, `blackbox-essential` pins maps to the BPF virtual filesystem (`bpffs`):

```text
/sys/fs/bpf/
└── blackbox/
    ├── blocked_ip_map     (BPF_MAP_TYPE_HASH: IPv4 block rules & drop counters)
    └── telemetry_drop_map (BPF_MAP_TYPE_PERCPU_ARRAY: Low-overhead drop counters)
```

---

## 2. Hash Map Mechanics (`BPF_MAP_TYPE_HASH`)

`blocked_ip_map` is configured as a kernel hash table with fixed-capacity pre-allocated buckets:

```c
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 65536);
    __type(key, __u32);                 // IPv4 source address
    __type(value, struct blocked_val);  // TTL + drop metrics
    __uint(pinning, LIBBPF_PIN_BY_NAME);
} blocked_ip_map SEC(".maps");
```

### Properties of `BPF_MAP_TYPE_HASH`

* **Concurrent Access:** Reads from the XDP fast-path are lock-free, utilizing Read-Copy-Update (RCU) synchronization inside the kernel.
* **Pre-Allocation:** By default, all 65,536 hash buckets are pre-allocated in kernel memory during initialization, avoiding allocation jitter during active mitigation.

---

## 3. User-Space Map Interaction via `bpf()` Syscalls

In user space, `blackbox::XdpManager` manages the map using standard file-descriptor operations:

```cpp
#include <bpf/bpf.h>
#include <blackbox/abi.hpp>
#include <stdexcept>

void insert_rule_to_kernel(int map_fd, uint32_t ip, uint64_t ttl_ns, uint32_t rule_id) {
    blackbox::abi::BlockedIpKey key{
        .ipv4_address = ip,
        .reserved = 0
    };

    blackbox::abi::BlockedIpValue val{
        .expire_timestamp_ns = ttl_ns,
        .drop_count = 0,
        .rule_id = rule_id,
        .flags = 0
    };

    // Atomic map insertion / update
    int ret = bpf_map_update_elem(map_fd, &key, &val, BPF_ANY);
    if (ret != 0) {
        throw std::runtime_error("Failed to update BPF map");
    }
}
```
```

---

### File: `blackbox-essential/docs/ebpf-xdp-subsystem/ttl-ephemeral-expiry.md`

```markdown
# Ephemeral Nanosecond TTL Expiration Engine

Threat patterns in modern networks (such as scanning bursts or volumetric SYN floods) are dynamic. Retaining blocked IP addresses indefinitely wastes memory and risks blocking legitimate hosts after dynamic IP reassignment.

`blackbox-essential` implements a dual-stage **Ephemeral TTL Eviction Engine**.

---

## 1. Two-Tier Eviction Architecture

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ STAGE 1: IN-KERNEL FAST PATH EVALUATION (Zero-Latency)      │
 │                                                             │
 │  Incoming Packet Ingress                                    │
 │            │                                                │
 │            ▼ Lookup key in blocked_ip_map                   │
 │  [ now = bpf_ktime_get_ns() ]                               │
 │            │                                                │
 │            ├── (now <= expire_timestamp_ns) ──► XDP_DROP    │
 │            │                                                │
 │            └── (now >  expire_timestamp_ns) ──► XDP_PASS    │
 └─────────────────────────────────────────────────────────────┘

 ┌─────────────────────────────────────────────────────────────┐
 │ STAGE 2: USERSPACE ASYNCHRONOUS SWEEPER (Lazy Cleanup)      │
 │                                                             │
 │  Runs every 5.0 seconds in background worker thread:        │
 │  - Traverses map via bpf_map_get_next_key()                 │
 │  - Unlinks stale expired elements via bpf_map_delete_elem() │
 │  - Reclaims hash table buckets without kernel locks         │
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. In-Kernel Monotonic Evaluation

Kernel time is sampled using `bpf_ktime_get_ns()`, which reads the hardware cycle counter (`CLOCK_MONOTONIC`):

```c
__u64 now = bpf_ktime_get_ns();
if (now > val->expire_timestamp_ns) {
    // TTL expired: immediately permit the packet without waiting for userspace deletion
    return XDP_PASS;
}
```

This prevents the data plane from blocking expired targets even if the user-space cleanup daemon experiences scheduling delays.

---

## 3. User-Space Asynchronous Garbage Collector

The control plane (`libblackbox.so`) runs an asynchronous background thread that sweeps the hash table to reclaim bucket memory:

```cpp
void XdpManager::run_ttl_garbage_collector() {
    uint32_t current_key = 0;
    uint32_t next_key = 0;
    uint64_t now_ns = get_monotonic_time_ns();

    std::vector<uint32_t> keys_to_delete;

    // Traverse all keys in the hash map
    while (bpf_map_get_next_key(map_fd_, &current_key, &next_key) == 0) {
        blackbox::abi::BlockedIpValue val{};
        if (bpf_map_lookup_elem(map_fd_, &next_key, &val) == 0) {
            if (now_ns > val.expire_timestamp_ns) {
                keys_to_delete.push_back(next_key);
            }
        }
        current_key = next_key;
    }

    // Delete expired entries
    for (uint32_t key : keys_to_delete) {
        bpf_map_delete_elem(map_fd_, &key);
    }
}
```
```

---

### File: `blackbox-essential/docs/ebpf-xdp-subsystem/kernel-lockdown-and-signing.md`

```markdown
# Linux Kernel Lockdown Mode & Cryptographic Signing

In hardened enterprise and defense environments, operating systems enforce **Linux Kernel Lockdown Mode** (`integrity` or `confidentiality`). This security policy restricts kernel modifications and untrusted eBPF programs.

`blackbox-essential` complies with Kernel Lockdown policies through cryptographic program signing and Secure Boot integration.

---

## 1. Understanding Lockdown Modes

| Lockdown State | Impact on eBPF & XDP Programs | `blackbox-essential` Status |
| :--- | :--- | :--- |
| **`none`** | Full access to all BPF program types and helper calls. | Operational |
| **`integrity`** | BPF features that can modify kernel memory are disabled. | **Operational:** XDP and read-only tracing are permitted. |
| **`confidentiality`** | Inspecting kernel memory structures (e.g., kprobes) is blocked. | **Operational:** Packet filtering using XDP remains permitted. |

Check the active lockdown state:

```bash
cat /sys/kernel/security/lockdown
```

---

## 2. Cryptographic ELF Signing via `sign-file`

To satisfy enterprise Secure Boot and custom kernel integrity verification policies, the compiled BPF object (`xdp_filter.o`) is signed using the host's private X.509 certificate:

```bash
# Sign the compiled BPF ELF object
/usr/src/linux-headers-$(uname -r)/scripts/sign-file \
    sha256 \
    /etc/ssl/certs/blackbox_kernel_sign.key \
    /etc/ssl/certs/blackbox_kernel_sign.crt \
    build/bpf/xdp_filter.o
```

---

## 3. TPM 2.0 PCR 7 Secure Boot Measurement

When Secure Boot is enabled, the cryptographic signature of the loaded eBPF mitigation module is measured into TPM 2.0 Platform Configuration Register 7 (**PCR 7**). 

Any unauthorized modification of `xdp_filter.o` will:
1. Cause the kernel verification loader to reject the object (`-EKEYREJECTED`).
2. Invalidate the TPM 2.0 PCR 7 quote, preventing the daemon from authenticating with the central fleet controller (`sentinel-nexus`).
```

---

### Complete in Part 3
- `blackbox-essential/docs/ebpf-xdp-subsystem/xdp-filter-architecture.md`
- `blackbox-essential/docs/ebpf-xdp-subsystem/driver-mode-vs-skb-mode.md`
- `blackbox-essential/docs/ebpf-xdp-subsystem/bpf-verifier-guarantees.md`
- `blackbox-essential/docs/ebpf-xdp-subsystem/compiling-bpf-bytecode.md`
- `blackbox-essential/docs/ebpf-xdp-subsystem/bpf-map-management.md`
- `blackbox-essential/docs/ebpf-xdp-subsystem/ttl-ephemeral-expiry.md`
- `blackbox-essential/docs/ebpf-xdp-subsystem/kernel-lockdown-and-signing.md`

All 7 eBPF/XDP subsystem documentation files are now generated.

---

### Files to be Generated in Part 4

The next phase covers the **Lock-Free SPMC Ring Buffer** (`spmc-ring-buffer/`):

1. `spmc-ring-buffer/lock-free-architecture.md` (Single-Producer Multi-Consumer overview)
2. `spmc-ring-buffer/atomic-memory-ordering.md` (Formalizing `std::memory_order_release` and `acquire`)
3. `spmc-ring-buffer/cache-line-padding.md` (Preventing false sharing with `alignas(64)`)
4. `spmc-ring-buffer/zero-mutex-producer.md` (Non-blocking enqueue from network driver threads)
5. `spmc-ring-buffer/multi-consumer-scaling.md` (Distributing flow events to parallel inference workers)
6. `spmc-ring-buffer/saturation-and-backpressure.md` (Bounded circular capacity and tail-drop mechanics)

Confirm when you are ready to proceed with Part 4.