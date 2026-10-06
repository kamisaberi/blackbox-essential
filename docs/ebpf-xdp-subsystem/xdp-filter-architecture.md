# XDP Filter Architecture & Packet Lifecycle

The in-kernel data plane of `blackbox-essential` is implemented in `bpf/xdp_filter.c`. It attaches to the network driver ingress hook and evaluates every incoming frame before memory is allocated for socket buffers (`sk_buff`).

---

## 1. Frame Ingress Lifecycle

```text
 [ Physical Wire / Fiber Optic Transceiver ]
                     │
                     ▼ Direct NIC DMA write to host RAM page
 [ Driver Rx Ring Buffer Descriptor ]
                     │
                     ▼ Driver triggers xdp_threat_filter(struct xdp_md *ctx)
 ┌─────────────────────────────────────────────────────────────┐
 │ 1. Boundary Initialization: data, data_end pointers         │
 └─────────────────────────────┬───────────────────────────────┘
                               │
                               ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ 2. Ethernet Parsing: Check protocol (0x0800 IPv4, 0x8100 VLAN)
 └─────────────────────────────┬───────────────────────────────┘
                               │
                               ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ 3. IPv4 Bounds & Header Verification (IHL >= 5, Total Length)│
 └─────────────────────────────┬───────────────────────────────┘
                               │
                               ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ 4. In-Kernel BPF Map Lookup: bpf_map_lookup_elem(blocked_ip) │
 └──────────────┬──────────────────────────────┬───────────────┘
                │                              │
                │ MATCH FOUND                  │ NO MATCH / EXPIRED
                ▼                              ▼
 ┌─────────────────────────────┐ ┌─────────────────────────────┐
 │ 5. Evaluate Nanosecond TTL  │ │ 7. Increment Clean Telemetry│
 │    now = bpf_ktime_get_ns() │ │    Counter                  │
 └──────────────┬──────────────┘ └─────────────┬───────────────┘
                │                              │
        ┌───────┴───────┐                      │
        │ EXPIRED       │ ACTIVE               │
        ▼               ▼                      ▼
  [ XDP_PASS ]    [ XDP_DROP ]           [ XDP_PASS ]
  (Forward to     (Recycle RX            (Forward to
   Linux Stack)    Descriptor)            Linux Stack)
```

---

## 2. In-Kernel C Implementation (`bpf/xdp_filter.c`)

```c
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

struct blocked_val {
    __u64 expire_timestamp_ns;
    __u64 drop_count;
    __u32 rule_id;
    __u32 flags;
};

// Pinned BPF Hash Map holding blocked IPv4 addresses
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 65536);
    __type(key, __u32); // IPv4 address in network byte order
    __type(value, struct blocked_val);
    __uint(pinning, LIBBPF_PIN_BY_NAME);
} blocked_ip_map SEC(".maps");

// Per-CPU Telemetry metrics
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __u64); // Slot 0: Total Drops
} telemetry_drop_map SEC(".maps");

SEC("xdp")
int xdp_threat_filter(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    // 1. Validate Ethernet Header Bounds
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end) {
        return XDP_PASS;
    }

    // 2. Parse IPv4 (Handle standard Ethernet frames)
    if (eth->h_proto != bpf_htons(ETH_P_IP)) {
        return XDP_PASS;
    }

    // 3. Validate IPv4 Header Bounds
    struct iphdr *ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end) {
        return XDP_PASS;
    }

    // Enforce IHL (Internet Header Length) bounds
    if (ip->ihl < 5) {
        return XDP_PASS;
    }
    if ((void *)((char *)ip + (ip->ihl * 4)) > data_end) {
        return XDP_PASS;
    }

    __u32 src_ip = ip->saddr;

    // 4. Lookup Source IP in In-Kernel Map
    struct blocked_val *val = bpf_map_lookup_elem(&blocked_ip_map, &src_ip);
    if (!val) {
        return XDP_PASS; // Clean IP: Pass to host kernel stack
    }

    // 5. Evaluate Ephemeral Nanosecond TTL
    __u64 now = bpf_ktime_get_ns();
    if (now > val->expire_timestamp_ns) {
        // Entry expired: allow pass (userspace will lazily clean up)
        return XDP_PASS;
    }

    // 6. Fast-Path Mitigation: Increment counters and DROP
    __sync_fetch_and_add(&val->drop_count, 1);

    __u32 key = 0;
    __u64 *total_drops = bpf_map_lookup_elem(&telemetry_drop_map, &key);
    if (total_drops) {
        *total_drops += 1;
    }

    // Discard frame immediately at the NIC driver layer
    return XDP_DROP;
}

char _license[] SEC("license") = "Dual BSD/GPL";
```

---

## 3. Supported Return Actions

| XDP Action Code | Value | Meaning inside `blackbox-essential` |
| :--- | :--- | :--- |
| `XDP_DROP` | `1` | Discard frame immediately. Driver recycles descriptor. $< 0.84\,\mu\text{s}$ mitigation. |
| `XDP_PASS` | `2` | Packet is clean. Allocate `sk_buff` and pass to standard Linux network stack. |
| `XDP_TX` | `3` | Bounce packet back out the same interface (used for TCP Reset reflection). |
| `XDP_REDIRECT` | `4` | Forward frame directly to an AF_XDP user-space socket for deep inspection. |
| `XDP_ABORTED` | `0` | Reserved for eBPF program errors (triggers driver tracepoint warning). |

