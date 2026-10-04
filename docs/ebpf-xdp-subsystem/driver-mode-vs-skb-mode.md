# Driver Mode vs. SKB Mode

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Native XDP (XDP_FLAGS_DRV_MODE) versus Generic SKB fallback.

## Native

Physical 10/25GbE NICs: 0.84µs verified SLA on Intel X520/E810 and ConnectX.

## SKB

VMware/KVM/veth: full functionality under 2.5µs, selected automatically.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
