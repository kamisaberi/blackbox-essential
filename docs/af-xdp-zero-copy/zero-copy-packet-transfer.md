# Zero-Copy Packet Transfer

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Zero-copy DMA transfers from NIC directly to inference memory.

## Path

DMA descriptor → UMEM slot → tensor view. No copy, no syscall, no context switch.

## Proof

Perf counters show zero memcpy calls during sustained 1.25M EPS.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
