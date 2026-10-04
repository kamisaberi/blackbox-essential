# Kernel–Userspace ABI

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


ABI stability, BPF map descriptors, and syscall boundaries.

## Maps

Pinned at /sys/fs/bpf/blackbox/ with stable key/value layouts across releases.

## Syscalls

A fixed ioctl surface; everything else crosses via maps and rings.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
