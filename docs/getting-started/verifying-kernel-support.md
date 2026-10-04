# Verifying Kernel Support

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Validating eBPF JIT, BTF, bpffs, and XDP driver support.

## Checks

JIT enabled, BTF present for the running kernel, bpffs mounted, driver XDP support probed per interface.

## Fallbacks

Missing native XDP drops to Generic SKB mode automatically; missing BTF disables CO-RE with a warning.

```bash
$ sysctl net.core.bpf_jit_enable   # want 1
$ ls /sys/kernel/btf/vmlinux
$ mount | grep bpffs
$ ./build/xdp_probe --iface eth0
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
