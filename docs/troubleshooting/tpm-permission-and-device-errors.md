# TPM Permission & Device Errors

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Fixing /dev/tpmrm0 access denied and missing resource manager issues.

## Permissions

tss group membership or udev rule; never chmod 666 the device node.

## Missing RM

Old kernels lack tpmrm; upgrade or use direct /dev/tpm0 with locking.

```bash
$ ls -l /dev/tpmrm0
$ groups | grep -o tss
$ dmesg | grep -i tpm | tail
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
