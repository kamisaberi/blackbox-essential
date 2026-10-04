# Lock-Free SPMC Architecture

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Single-producer multi-consumer ring buffer overview: 256k slots, zero mutexes.

## Roles

One AF_XDP poll thread produces; N inference workers consume via CAS claims.

## Capacity

262,144 descriptors absorb line-rate bursts without drops.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
