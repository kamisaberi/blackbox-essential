# Multi-Consumer Scaling

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Distributing flow events to parallel inference worker threads.

## Claiming

Workers CAS-compete for slots; losers retry without sleeping.

## Affinity

Pin one worker per core; throughput scales linearly to core count.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
