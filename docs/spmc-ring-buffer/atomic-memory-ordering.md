# Formalizing Atomic Memory Ordering (`acquire` & `release`)

Using default sequential consistency (`std::memory_order_seq_cst`) introduces full hardware memory barriers (`mfence` on x86, `dmb ish` on ARM64), stalling processor pipeline execution.

`blackbox::EventRingBuffer` uses fine-grained **Acquire-Release memory ordering** to synchronize data visibility across CPU cores with minimal cycle overhead.

---

## 1. The Acquire-Release Synchronization Pair

```text
PRODUCER THREAD (Core A)                     CONSUMER THREAD (Core B)
 1. Write payload into slot:                  1. Read write_index with acquire:
    slots_[idx].payload = event;                 uint64_t w = write_idx_.load(acquire);
 2. Store write_index with release:           2. CAS read_index with acquire:
    write_idx_.store(next, release);             read_idx_.compare_exchange_weak(acquire);
                                              3. Read payload safely:
                                                 event = slots_[idx].payload;
```

---

## 2. Mathematical Proof of Correctness

* **Producer Invariant:** Storing `write_idx_` using `std::memory_order_release` guarantees that all prior memory stores (populating the `Event` payload in the buffer slot) are visible to any thread that loads `write_idx_` with `std::memory_order_acquire`.
* **Consumer Invariant:** Loading `write_idx_` using `std::memory_order_acquire` guarantees that subsequent reads of the slot contents cannot be reordered before the index check by the CPU instruction pipeline.

---

## 3. C++20 Atomic Implementation Snippet

```cpp
#include <atomic>
#include <cstdint>
#include <span>

namespace blackbox {

struct alignas(64) BufferSlot {
    std::atomic<uint64_t> sequence{0};
    FlowEvent payload{};
};

class EventRingBuffer {
    // Producer publishes an event
    bool try_enqueue(const FlowEvent& event) noexcept {
        const uint64_t current_tail = write_index_.load(std::memory_order_relaxed);
        const uint64_t current_head = read_index_.load(std::memory_order_acquire);

        // Check if ring is full
        if (current_tail - current_head >= capacity_) {
            dropped_count_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        BufferSlot& slot = slots_[current_tail & index_mask_];
        
        // Write data into pre-allocated memory
        slot.payload = event;

        // Release barrier: makes payload writes visible before advancing index
        write_index_.store(current_tail + 1, std::memory_order_release);
        return true;
    }
};

} // namespace blackbox
```

