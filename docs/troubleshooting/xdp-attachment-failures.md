# XDP Attachment Failures

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Resolving Operation not supported and driver attachment errors.

## Support

Confirm with the probe tool; fall back to SKB mode on vNICs.

## Lockdown

Integrity-mode kernels need signed objects — see the signing page.

```bash
$ ./build/xdp_probe --iface eth0
$ ip link set dev eth0 xdp obj xdp_filter.o sec xdp
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
