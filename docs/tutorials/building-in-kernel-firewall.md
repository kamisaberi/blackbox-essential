# In-Kernel Firewall in 50 Lines

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Creating a wire-speed packet blocker in 50 lines of C++20.

## Steps

Attach → insert one rule → read counters. under fifty lines total.

## Verify

Counter increments prove kernel enforcement, not userspace luck.

```cpp
// Full tutorial builds this live; the verdict line:
// return XDP_DROP;  // 0.84us, driver ring only
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
