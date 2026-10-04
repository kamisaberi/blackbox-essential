# Wiring XDP to xInfer

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Connecting eBPF packet capture to xInfer neural scoring.

## Handoff

AF_XDP descriptors feed the assembler plugin; vectors score in libxinfer.

## Verdict loop

High scores program blocked_ip_map — the same map the filter reads.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
