# Core Engine Design

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Decoupled Tier 2 architecture and the resolve-to-enforce execution pipeline.

## Pipeline

Attach → map → ring → score → enforce. Kernel decides; userspace explains.

## Decoupling

Filter, transport, identity, and scoring share nothing but file descriptors and atomics.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
