#!/usr/bin/env bash
set -e

echo "Building eBPF XDP Kernel Kernel Filters for Ubuntu..."
clang -O2 -target bpf -c src/mitigation/xdp_filter.c -o build/xdp_filter.o
echo "eBPF Kernel filter compiled successfully: build/xdp_filter.o"