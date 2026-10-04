# Compiling BPF Bytecode

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Clang/LLVM flags (-O2 -target bpf) and the build_bpf.sh pipeline.

## Flags

-O2 -target bpf -g for BTF; -Wall catches most verifier rejections early.

## Script

build_bpf.sh compiles, verifies with bpftool, and stages xdp_filter.o.

```bash
$ clang -O2 -target bpf -g -c bpf/xdp_filter.c -o bpf/xdp_filter.o
$ bpftool prog load bpf/xdp_filter.o /sys/fs/bpf/xdp_filter
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
