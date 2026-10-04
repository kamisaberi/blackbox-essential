# Saturation & Backpressure

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Bounded circular capacity handling and tail-drop mechanics.

## Policy

Full ring drops at the tail with a counter — bursts degrade gracefully, never deadlock.

## Observability

Drop counters export per second for capacity planning.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
