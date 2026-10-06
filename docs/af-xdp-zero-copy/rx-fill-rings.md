# Coordinating Descriptor Exchanges: The 4 AF_XDP Rings

AF_XDP coordinates memory handoffs between the kernel driver and user space using **four lock-free circular ring buffers**:

```text
 ┌─────────────────────────────────────────────────────────────────────────────┐
 │                               USER SPACE                                    │
 └──────────────────────┬──────────────────────────────▲───────────────────────┘
                        │                              │
         (1) FILL RING: │               (2) RX RING:   │
         Passes empty   │               Delivers full  │
         UMEM chunk     │               packet frames  │
         addresses to   │               to user space  │
         the driver     ▼                              │
 ┌─────────────────────────────────────────────────────┴───────────────────────┐
 │                            KERNEL DRIVER SPACE                              │
 └──────────────────────┬──────────────────────────────▲───────────────────────┘
                        │                              │
         (3) TX RING:   │         (4) COMPLETION RING: │
         Submits packet │             Signals sent     │
         addresses for  │             packets can be   │
         hardware       │             reused in user   │
         transmission   ▼             space            │
 ┌─────────────────────────────────────────────────────┴───────────────────────┐
 │                            NIC HARDWARE ASIC                                │
 └─────────────────────────────────────────────────────────────────────────────┘
```

---

## 1. Ring Descriptions

| Ring Name | Producer | Consumer | Function |
| :--- | :--- | :--- | :--- |
| **FILL Ring** | User Space | Kernel Driver | Hands empty UMEM chunk addresses to the driver for incoming DMA. |
| **RX Ring** | Kernel Driver | User Space | Delivers metadata (offset, length) of newly arrived packet frames. |
| **TX Ring** | User Space | Kernel Driver | Hands completed frames to the driver for egress transmission. |
| **COMPLETION Ring** | Kernel Driver | User Space | Informs user space that a transmitted chunk has left the wire. |

---

## 2. Ingestion Processing Loop in C++20

The consumer loop receives packets from the **RX ring**, processes or scores them, and returns the chunk back to the **FILL ring**:

```cpp
#include <xdp/xsk.h>
#include <blackbox/flow_event.hpp>
#include <iostream>

void process_ingress_packets(
    struct xsk_ring_cons* rx_ring,
    struct xsk_ring_prod* fill_ring,
    uint8_t* umem_buffer,
    size_t batch_size
) {
    uint32_t idx_rx = 0;
    uint32_t idx_fill = 0;

    // 1. Peek at newly received packets in RX ring
    uint32_t rcvd = xsk_ring_cons__peek(rx_ring, batch_size, &idx_rx);
    if (rcvd == 0) {
        return; // No packets pending
    }

    // 2. Reserve slots in the FILL ring to recycle chunks immediately
    xsk_ring_prod__reserve(fill_ring, rcvd, &idx_fill);

    for (uint32_t i = 0; i < rcvd; ++i) {
        // Read descriptor from RX ring
        const struct xdp_desc* desc = xsk_ring_cons__rx_desc(rx_ring, idx_rx + i);
        uint64_t addr = desc->addr;
        uint32_t len = desc->len;

        // Direct memory pointer to raw Ethernet frame (Zero-Copy)
        const uint8_t* pkt_data = umem_buffer + addr;

        // Execute parsing or scoring hook
        handle_packet(pkt_data, len);

        // Recycle chunk address back to FILL ring for next incoming packet
        *xsk_ring_prod__fill_addr(fill_ring, idx_fill + i) = addr;
    }

    // 3. Release consumed descriptors and submit replenished fill slots
    xsk_ring_cons__release(rx_ring, rcvd);
    xsk_ring_prod__submit(fill_ring, rcvd);
}
```

---

## 3. Wakeup Flag Optimization (`XSK_RING_NEED_WAKEUP`)

When `XSK_UMEM__USES_NEED_WAKEUP` is active, the kernel driver sets the `NEED_WAKEUP` flag on the Fill or Tx rings when its internal queues stall. User space checks this flag and calls `poll()` or `sendto()` only when necessary, eliminating redundant system call overhead.

