# UMEM Architecture & Memory Pool Allocation

AF_XDP (Address Family XDP, formerly XSK) provides low-latency, high-throughput packet streaming between the Linux kernel and user space. At the core of AF_XDP is the **UMEM (User Memory)** area: a pre-allocated, memory-mapped virtual memory pool shared between the user-space process and the network interface card (NIC) driver.

---

## 1. UMEM Memory Topology

A UMEM is allocated as a contiguous memory block in user-space RAM, divided into fixed-size regions called **chunks** (typically 2048 or 4096 bytes, matching standard page or jumbo frame boundaries):

```text
 ┌─────────────────────────────────────────────────────────────────────────────┐
 │                     blackbox::UmemArea (Host RAM)                           │
 │   - Size: 128 MB (32,768 Chunks x 4096 Bytes)                               │
 │   - Aligned to 4096-Byte Page Boundaries (posix_memalign / mmap)            │
 └─────────────────────────────────────────────────────────────────────────────┘
      │ Chunk 0          │ Chunk 1          │ Chunk 2          │ Chunk 32767
      ▼ (4096 Bytes)     ▼ (4096 Bytes)     ▼ (4096 Bytes)     ▼ (4096 Bytes)
 ┌──────────────────┬──────────────────┬──────────────────┬─────── ... ────────┐
 │ Packet Header    │ Packet Header    │ Packet Header    │ Packet Header      │
 │ & Frame Payload  │ & Frame Payload  │ & Frame Payload  │ & Frame Payload    │
 └──────────────────┴──────────────────┴──────────────────┴────────────────────┘
      ▲                  ▲                  ▲                  ▲
      │ Direct DMA Write │ Direct DMA Write │ Direct DMA Write │ Direct DMA Write
 ┌─────────────────────────────────────────────────────────────────────────────┐
 │              Network Interface Card (NIC) Hardware DMA Controller           │
 └─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Chunk Addressing: Aligned vs. Unaligned Mode

`blackbox-essential` supports both UMEM addressing modes:

1. **Aligned Mode:** Chunk addresses are strict multiples of the chunk size:
   $$\text{Addr} = k \times \text{ChunkSize}$$
   * Lower complexity; direct base-offset indexing.
2. **Unaligned Mode (Recommended):** Allows packet data to start at arbitrary offsets within a chunk:
   * Enables zero-copy packet header prepending and direct handoff to downstream frameworks without re-allocating memory.

---

## 3. Allocating and Registering UMEM in C++20

```cpp
#include <xdp/xsk.h>
#include <sys/mman.h>
#include <cstdlib>
#include <stdexcept>
#include <cstdint>

namespace blackbox {

class UmemPool {
public:
    UmemPool(size_t chunk_count, size_t chunk_size)
        : chunk_count_(chunk_count), chunk_size_(chunk_size) {
        
        size_t total_bytes = chunk_count_ * chunk_size_;

        // 1. Allocate page-aligned anonymous memory
        buffer_ = ::mmap(
            nullptr,
            total_bytes,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
            -1,
            0
        );

        if (buffer_ == MAP_FAILED) {
            throw std::runtime_error("Failed to allocate UMEM buffer via mmap");
        }

        // 2. Lock memory pages to prevent swapping
        ::mlock(buffer_, total_bytes);

        // 3. Configure UMEM registration structure
        struct xsk_umem_config config = {
            .fill_size = static_cast<uint32_t>(chunk_count_ / 2),
            .comp_size = static_cast<uint32_t>(chunk_count_ / 2),
            .frame_size = static_cast<uint32_t>(chunk_size_),
            .frame_headroom = XSK_UMEM__DEFAULT_FRAME_HEADROOM,
            .flags = XSK_UMEM__USES_NEED_WAKEUP
        };

        // 4. Create UMEM handle via libxdp / libbpf
        int ret = xsk_umem__create(
            &umem_,
            buffer_,
            total_bytes,
            &fill_ring_,
            &comp_ring_,
            &config
        );

        if (ret != 0) {
            ::munmap(buffer_, total_bytes);
            throw std::runtime_error("xsk_umem__create failed with code: " + std::to_string(ret));
        }
    }

    ~UmemPool() {
        if (umem_) {
            xsk_umem__delete(umem_);
        }
        if (buffer_ && buffer_ != MAP_FAILED) {
            ::munmap(buffer_, chunk_count_ * chunk_size_);
        }
    }

    [[nodiscard]] void* get_buffer() const noexcept { return buffer_; }
    [[nodiscard]] struct xsk_umem* get_umem() const noexcept { return umem_; }

private:
    size_t chunk_count_{0};
    size_t chunk_size_{4096};
    void* buffer_{nullptr};
    struct xsk_umem* umem_{nullptr};
    struct xsk_ring_prod fill_ring_{};
    struct xsk_ring_cons comp_ring_{};
};

} // namespace blackbox
```

