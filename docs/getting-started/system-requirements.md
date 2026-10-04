# System Requirements

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Linux kernel 5.15+, eBPF JIT, Clang/LLVM, libelf, and capable NICs.

## Kernel

5.15 through 6.11 with BPF JIT enabled, BTF available, and bpffs mounted at /sys/fs/bpf.

## Toolchain

Clang 16+, LLVM, libelf-dev, libssl-dev, CMake 3.20+. Root or CAP_BPF for loading programs.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
