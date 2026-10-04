# Atomic Memory Ordering

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Formalizing memory_order_release and memory_order_acquire across the ring.

## Producer

Payload store, then tail release — consumers never see torn slots.

## Consumers

Acquire-load the tail; CAS-claim slots; release on completion.

```cpp
tail_.store(next, std::memory_order_release);  // publish
// ...
auto t = tail_.load(std::memory_order_acquire);  // observe
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
