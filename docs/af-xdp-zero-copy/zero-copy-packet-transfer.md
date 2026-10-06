# Zero-Copy DMA Transfers from NIC Directly to Inference Memory

In standard Linux packet capture (e.g., `libpcap`, raw sockets), packets are copied up to three times across kernel and user-space boundaries. 

With **AF_XDP Zero-Copy Mode (`XDP_ZEROCOPY`)**, the NIC hardware DMA controller deposits frames directly into user-space allocated UMEM chunks.

---

## 1. Copy Mode vs. Zero-Copy Mode

```text
STANDARD AF_XDP (Copy Mode):
[ NIC DMA ] ──► [ Driver Memory Page ] ──(memcpy)──► [ User Space UMEM Chunk ]
                                                           │
                                                           ▼ CPU Overhead

--------------------------------------------------------------------------------

NATIVE AF_XDP ZERO-COPY (XDP_ZEROCOPY):
[ NIC DMA ] ═════════════════════════════════════════► [ User Space UMEM Chunk ]
                                                           │
                                                           ▼ Direct Pointer Hand-off
                                                   [ xinfer::Tensor Input View ]
```

---

## 2. In-Kernel Redirect eBPF Program

To route packets into an AF_XDP socket without traversing the kernel stack, an eBPF program uses `bpf_redirect_map()`:

```c
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

// BPF Map storing AF_XDP socket file descriptors per queue
struct {
    __uint(type, BPF_MAP_TYPE_XSKMAP);
    __uint(max_entries, 64);
    __type(key, __u32);   // Hardware RX Queue ID
    __type(value, __u32); // Socket file descriptor
} xsks_map SEC(".maps");

SEC("xdp")
int xdp_redirect_to_userspace(struct xdp_md *ctx) {
    __u32 queue_id = ctx->rx_queue_index;

    // Check if an AF_XDP socket is bound to this RX queue
    if (bpf_map_lookup_elem(&xsks_map, &queue_id)) {
        // Direct hardware zero-copy bypass into user space
        return bpf_redirect_map(&xsks_map, queue_id, 0);
    }

    return XDP_PASS;
}

char _license[] SEC("license") = "Dual BSD/GPL";
```

---

## 3. Direct Binding to `xinfer-essential`

Once the packet arrives in the UMEM chunk, `blackbox-essential` wraps the raw memory address into an `xinfer::Tensor` handle without allocating intermediate heap memory:

```cpp
#include <xinfer/tensor.hpp>

// Wrap chunk memory directly into an inference tensor view
xinfer::TensorDescriptor desc{
    .dimensions = {1, 32},
    .precision = xinfer::Precision::FP32,
    .memory_type = xinfer::MemoryType::HOST_PINNED
};

// Pointer aliasing: Zero memory copy between networking and AI layers
auto tensor = xinfer::Tensor::create_from_raw_host(
    desc,
    const_cast<uint8_t*>(pkt_data + 14), // Skip Ethernet header directly to IP/TCP
    32 * sizeof(float)
);

engine.bind_input("flow_vector", tensor);
engine.forward();
```

