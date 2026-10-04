# TTL Ephemeral Expiry

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Nanosecond timestamp eviction logic executed in kernel space.

## Rule

Each entry carries an absolute expiry in nanoseconds from bpf_ktime_get_ns().

## Eviction

Expired lookups delete inline; a sweeper reaps stragglers under memory pressure.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
