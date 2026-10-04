# XDP vs. iptables/nftables

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Driver-level XDP against Linux Netfilter: 0.84µs vs 14.2µs vs 9.8µs.

## Why XDP wins

No stack traversal, no connection tracking, no table walks on the verdict path.

## When Netfilter wins

Complex stateful policies with conntrack already paid for.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
