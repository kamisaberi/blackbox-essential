# XDP vs. Suricata NFQUEUE

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Driver-level XDP against userspace NFQUEUE: 0.84µs vs 8.4ms — four orders of magnitude.

## Cost of userspace

Two context switches plus queueing dominate; detection quality is a separate question.

## Hybrid

Mirror to Suricata for forensics while XDP enforces — best of both.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
