# CPU Cycle Profiling

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Measuring CPU cycles per packet drop: under 120 cycles on the verdict path.

## Method

RDTSC around map lookup plus branch; median of 1M samples.

## Budget

120 cycles leaves headroom for parsing before the microsecond budget breaks.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
