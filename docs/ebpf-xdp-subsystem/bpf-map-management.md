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

