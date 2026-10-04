# Technical FAQ

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Short answers to recurring bring-up questions.

## Slow in VM?

Expected: SKB mode trades ~2µs for universality. Bare metal holds 0.84µs.

## No TPM?

Tier 3 DMI fallback engages and Nexus marks the node degraded — by design.

## Which interface?

The span/mirror port for sensing; management stays on a separate NIC.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
