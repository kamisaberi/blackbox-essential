# Class `blackbox::EventRingBuffer`

Defined in header `<blackbox/event_ring_buffer.hpp>`  
Namespace: `blackbox`

`EventRingBuffer` is a lock-free Single-Producer Multi-Consumer (SPMC) circular queue designed to stream network flow telemetry from driver ingestion threads to parallel worker pools without mutex locks.

---

## 1. Class Synopsis

```cpp
namespace blackbox {

class BLACKBOX_API EventRingBuffer {
public:
    explicit EventRingBuffer(size_t capacity = 65536);
    ~EventRingBuffer();

    // Non-copyable, non-movable
    EventRingBuffer(const EventRingBuffer&) = delete;
    EventRingBuffer& operator=(const EventRingBuffer&) = delete;
    EventRingBuffer(EventRingBuffer&&) = delete;
    EventRingBuffer& operator=(EventRingBuffer&&) = delete;

    // Single-Producer Interface (Wait-Free)
    bool try_enqueue(const FlowEvent& event) noexcept;

    // Multi-Consumer Interface (Lock-Free CAS)
    bool try_dequeue(FlowEvent& out_event) noexcept;

    // Introspection & Capacity
    [[nodiscard]] size_t capacity() const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] uint64_t dropped_count() const noexcept;
    [[nodiscard]] RingBufferMetrics get_metrics() const noexcept;

    // Lifecycle
    void reset() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace blackbox
```

---

## 2. Producer & Consumer Guarantees

### `try_enqueue`
```cpp
bool try_enqueue(const FlowEvent& event) noexcept;
```
Called exclusively by the single network driver ingestion thread. Evaluates in $O(1)$ time with zero system calls. If the ring is saturated, increments `dropped_count_` and returns `false` (Tail Drop).

---

### `try_dequeue`
```cpp
bool try_dequeue(FlowEvent& out_event) noexcept;
```
Safe for concurrent invocation by multiple worker threads. Uses an atomic Compare-And-Swap (CAS) loop on the read index. Returns `true` if an event was claimed and copied into `out_event`, or `false` if the ring is empty.

