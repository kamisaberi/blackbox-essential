# Multi-Consumer Scaling & Contention Arbitration

While only one producer writes to the ring, multiple worker threads running AI inference models (`xinfer-essential`) dequeue events simultaneously. 

Consumers arbitrate slot access using an atomic Compare-And-Swap (CAS) loop on `read_index_`.

---

## 1. Consumer CAS Arbitration Loop

```text
 Consumer Thread A                                Consumer Thread B
 ┌───────────────────────────────────────┐        ┌───────────────────────────────────────┐
 │ 1. Read current head:                 │        │ 1. Read current head:                 │
 │    head = read_index_.load(relaxed)   │        │    head = read_index_.load(relaxed)   │
 │ 2. Check if data available (head < w) │        │ 2. Check if data available (head < w) │
 │ 3. Attempt CAS:                       │        │ 3. Attempt CAS:                       │
 │    CAS(read_index_, head, head + 1)   │        │    CAS(read_index_, head, head + 1)   │
 └──────────────────┬────────────────────┘        └──────────────────┬────────────────────┘
                    │                                                │
         SUCCESS ───┴───► [ Claims Slot N ]               FAILS ─────┴──► [ Retries Loop ]
```

---

## 2. Implementation: `try_dequeue()`

```cpp
bool EventRingBuffer::try_dequeue(FlowEvent& out_event) noexcept {
    uint64_t head = read_index_.load(std::memory_order_relaxed);

    while (true) {
        const uint64_t tail = write_index_.load(std::memory_order_acquire);

        // Ring is empty
        if (head >= tail) {
            return false;
        }

        // Attempt to atomically claim this slot
        if (read_index_.compare_exchange_weak(
                head, 
                head + 1, 
                std::memory_order_acquire, 
                std::memory_order_relaxed)) {
            
            // Successfully claimed: copy payload out
            out_event = slots_[head & index_mask_].payload;
            return true;
        }
        // CAS failed: head is updated automatically by compare_exchange_weak; retry
    }
}
```

---

## 3. Resolving the ABA Problem

Traditional lock-free queues that store pointers suffer from the **ABA problem**, where an address is freed, reallocated, and misidentified as unchanged.

`blackbox::EventRingBuffer` avoids this problem by design:
* It uses **monotonic 64-bit sequence counters** (`uint64_t`).
* At a rate of $10{,}000{,}000\text{ EPS}$, a 64-bit counter will not overflow for over **$58{,}494\text{ years}$**. Sequence counters never repeat within the operating lifetime of the appliance.

