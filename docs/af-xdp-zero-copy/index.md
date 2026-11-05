# AF_XDP Zero-Copy Architecture

AF_XDP (Address Family XDP) provides direct packet streaming between NIC hardware and user-space memory pools without traversing the kernel TCP/IP stack.

## Section Navigation

- [UMEM Architecture](./umem-architecture.md): Memory pool allocation, 4096-byte chunk layouts, and aligned vs. unaligned modes.
- [Rx & Fill Rings](./rx-fill-rings.md): Coordinating memory handoffs across the 4 lock-free AF_XDP circular rings.
- [Zero-Copy Transfer](./zero-copy-packet-transfer.md): Hardware DMA transfers directly into user-space UMEM chunks and `xinfer::Tensor` aliasing.
- [10GbE Line-Rate Saturation](./line-rate-saturation-10gbe.md): NIC compatibility matrix, kernel sysctl optimizations, and saturation profiles.
- [Multi-Core RSS Queues](./multi-core-rss-queues.md): Scaling packet ingestion across multi-queue NICs using Receive Side Scaling.
