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

