# Tier 3: DMI Fallback

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Motherboard DMI product_uuid hashing and machine-id fallback for zero-TPM hardware.

## Seed

product_uuid + board_serial + machine-id, hashed with SHA-256.

## Posture

Explicitly marked degraded in Nexus — deterrence, not proof.

```bash
$ cat /sys/class/dmi/id/product_uuid
$ cat /sys/class/dmi/id/board_serial
$ sha256sum /etc/machine-id
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
