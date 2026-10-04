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