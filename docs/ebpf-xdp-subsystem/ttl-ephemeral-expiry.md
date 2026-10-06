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

