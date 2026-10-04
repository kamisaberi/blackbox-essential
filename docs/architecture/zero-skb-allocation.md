# Zero sk_buff Allocation

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Discarding frames before Linux kernel socket-buffer creation.

## Mechanism

XDP_DROP returns the page to the driver ring; no skb is ever allocated for hostile traffic.

## Effect

Dropped traffic costs < 120 CPU cycles and zero kernel heap.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
