# Lock-Free SPMC Architecture Overview

The `blackbox::EventRingBuffer` is an in-memory Single-Producer Multi-Consumer (SPMC) queue designed to transfer telemetry events from high-speed network driver threads to parallel machine learning inference workers without lock contention.

---

## 1. Concurrency Model: 1 Producer $\to$ N Consumers

In edge appliances, network frames are ingested sequentially by a single core servicing the NIC driver's interrupt queue (or an AF_XDP polling thread). However, threat scoring using neural networks (`xinfer-essential`) requires parallel processing across multiple CPU/NPU cores:

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ Single Producer: Network Driver Ingress Thread (Core 0)      │
 └──────────────────────────────┬──────────────────────────────┘
                                │ try_enqueue() (Wait-Free, No Mutex)
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │       blackbox::EventRingBuffer (Fixed 65,536 Slots)        │
 │  - Monotonic 64-bit Sequence Counters (write_idx, read_idx) │
 │  - Power-of-Two Bitmask Wrap: (idx & (CAPACITY - 1))        │
 │  - 64-Byte Cache-Line Isolated Heads and Tails              │
 └──────────────┬──────────────────────────────┬───────────────┘
                │ try_dequeue() (CAS Loop)     │ try_dequeue() (CAS Loop)
                ▼                              ▼
 ┌─────────────────────────────┐ ┌─────────────────────────────┐
 │ Inference Worker (Core 1)   │ │ Inference Worker (Core N)   │
 │ xinfer::InferenceEngine     │ │ xinfer::InferenceEngine     │
 └─────────────────────────────┘ └─────────────────────────────┘
```

---

## 2. Invariants for 1.25M+ EPS Throughput

1. **Wait-Free Producer:** The network driver thread never blocks, sleeps, or spins. If the ring buffer is saturated, it increments an atomic drop counter and returns immediately.
2. **Lock-Free Consumers:** Consumers claim events using atomic Compare-And-Swap (`compare_exchange_weak`) operations. Even if a consumer thread pauses, other consumers continue dequeuing without deadlocks.
3. **Zero Dynamic Allocation:** Buffer slots are pre-allocated during initialization. Processing events involves zero calls to `malloc()` or `free()`.
4. **Power-of-Two Modulo Elimination:** The buffer capacity ($C$) is strictly a power of two ($2^k$). Array indexing replaces expensive integer division with bitwise masking:

$$\text{Slot Index} = \text{Sequence Counter} \ \& \ (C - 1)$$

