# Zero-Mutex Producer

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Non-blocking enqueue operations from network driver threads.

## Fast path

Compare tail against head-acquire; full ring applies backpressure, never blocks.

## No syscalls

The producer never enters the kernel after setup.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
