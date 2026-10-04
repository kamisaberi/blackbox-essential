# Overview

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Core mission: sub-microsecond (< 0.84µs) active defense from inside the NIC driver.

## What it is

libblackbox.so is a Tier 2 security core: eBPF/XDP filtering, SPMC event transport, hardware attestation, and decoupled model binding.

## What it is not

Not a userspace firewall, not a cloud agent. Verdicts execute in the driver ring.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
