# RX & Fill Rings

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Coordinating descriptor exchanges between NIC and userspace.

## Flow

Userspace posts empty descriptors on FILL; the kernel returns frames on RX.

## Starvation

Monitor FILL depth; empty FILL rings are the top cause of mystery drops.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
