# Line-Rate Saturation on 10GbE

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Pushing 1.25M+ events/sec on Intel X520 and E810 adapters.

## Tuning

256 descriptors per queue, interrupt coalescing off, workers pinned per queue.

## Results

Sustained 1.25M EPS with p99 verdict latency under 1µs.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
