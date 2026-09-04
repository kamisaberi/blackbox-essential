# #!/usr/bin/env bash
# set -e

# echo "Compiling eBPF Kernel C code into BPF bytecode..."
# clang -O2 -target bpf -c src/mitigation/xdp_drop.c -o build/xdp_drop.o
# echo "eBPF bytecode successfully compiled to build/xdp_drop.o"



#!/usr/bin/env bash
set -e

ARCH=$(uname -m)
echo "Compiling eBPF XDP kernel bytecode for architecture: ${ARCH}..."

clang -O2 -target bpf \
      -I/usr/include/${ARCH}-linux-gnu \
      -I/usr/include \
      -c src/mitigation/xdp_drop.c \
      -o src/mitigation/xdp_drop.o

echo "eBPF bytecode successfully compiled to src/mitigation/xdp_drop.o"