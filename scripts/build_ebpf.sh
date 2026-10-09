#!/usr/bin/env bash
set -e

# Automatically resolve the blackbox-essential project root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${PROJECT_ROOT}"

ARCH=$(uname -m)
case "$ARCH" in
    x86_64)  BPF_ARCH="x86" ;;
    aarch64) BPF_ARCH="arm64" ;;
    *)       BPF_ARCH="$ARCH" ;;
esac

echo "[+] Project root: ${PROJECT_ROOT}"
echo "[+] Compiling eBPF CO-RE bytecode (Target: bpf, Arch: ${BPF_ARCH})..."

clang -g -O2 -target bpf \
      -D__TARGET_ARCH_${BPF_ARCH} \
      -Isrc/mitigation \
      -I/usr/include \
      -c src/mitigation/xdp_drop.c \
      -o src/mitigation/xdp_drop.o

echo "[SUCCESS] eBPF CO-RE bytecode successfully compiled to src/mitigation/xdp_drop.o"