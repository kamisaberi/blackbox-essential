# Lock-Free SPMC Event Ring Buffer

The `blackbox::EventRingBuffer` transfers flow telemetry from network driver threads to parallel machine learning inference workers without mutex locks.

## Section Navigation

- [Lock-Free Architecture](./lock-free-architecture.md): Concurrency model (1 Producer $\to$ N Consumers) and wait-free guarantees.
- [Atomic Memory Ordering](./atomic-memory-ordering.md): Fine-grained Acquire-Release (`acquire`/`release`) memory ordering proofs.
- [Cache-Line Padding](./cache-line-padding.md): Eliminating CPU false sharing using `alignas(64)` cache line isolation.
- [Zero-Mutex Producer](./zero-mutex-producer.md): $O(1)$ wait-free enqueue algorithm and thread-local cached head optimization.
- [Multi-Consumer Scaling](./multi-consumer-scaling.md): Atomic CAS arbitration loop and ABA problem resolution.
- [Saturation & Backpressure](./saturation-and-backpressure.md): Tail-drop mechanics and hardware backpressure signaling to eBPF maps.
