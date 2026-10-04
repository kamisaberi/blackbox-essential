# Mapping Tensor Dimensions

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Binding arbitrary input dimensions: 32-dim, 42-dim, 80-dim flows.

## Rule

Declare [batch, width] per model; the binder validates against the graph signature.

## Migration

Moving 32→80 dims is a config edit plus recalibration, not a recompile.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
