# BPF Map Management

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


BPF_MAP_TYPE_HASH mechanics, sizing, and pinned map namespaces.

## Sizing

500k entries default in non-pageable memory; tune per appliance class.

## Namespaces

Maps pin under /sys/fs/bpf/blackbox/ so reloads and debuggers share state.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
