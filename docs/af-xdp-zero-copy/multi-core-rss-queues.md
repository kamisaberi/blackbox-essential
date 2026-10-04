# Multi-Core RSS Queues

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Scaling across multi-queue NICs using Receive Side Scaling.

## Hashing

RSS spreads flows by 5-tuple hash so one flow always lands on one queue.

## Mapping

One queue, one worker, one core — no cross-core cache traffic.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
