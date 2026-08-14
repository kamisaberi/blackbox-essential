#!/usr/bin/env bash
set -e

echo "Compiling eBPF Kernel C code into BPF bytecode..."
clang -O2 -target bpf -c src/mitigation/xdp_drop.c -o build/xdp_drop.o
echo "eBPF bytecode successfully compiled to build/xdp_drop.o"