# XDP Filter Architecture

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Lifecycle of an incoming Ethernet frame inside xdp_threat_filter().

## Stages

Bounds proof → Ethernet/IP parse → map lookup → TTL check → DROP or PASS.

## Egress

Passed frames hand to the AF_XDP ring by descriptor for background scoring.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
