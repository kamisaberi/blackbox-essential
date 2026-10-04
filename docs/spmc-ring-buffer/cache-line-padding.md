# Cache-Line Padding

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Preventing false sharing with alignas(64) cache-line alignment.

## Rule

Head, tail, and slot batches each start on their own 64-byte line.

## Proof

Perf c2c shows zero cross-core invalidations on the hot path.

```cpp
alignas(64) std::atomic<uint64_t> write_cursor_{0};
alignas(64) std::atomic<uint64_t> read_cursor_{0};
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
