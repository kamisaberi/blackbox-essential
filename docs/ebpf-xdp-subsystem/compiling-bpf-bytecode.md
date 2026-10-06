# Compiling BPF Bytecode & Toolchain Pipeline

Compiling eBPF bytecode requires targeting the Clang/LLVM BPF virtual machine architecture (`-target bpf`). This document details the compilation pipeline that produces `xdp_filter.o`.

---

## 1. The Compilation Toolchain Pipeline

```text
 [ C Source Code: bpf/xdp_filter.c ]
                    │
                    ▼ clang-16 -target bpf -O2 -g
 [ LLVM Intermediate Representation (IR) ]
                    │
                    ▼ llvm-strip -g (Strip non-BTF debug sections)
 [ BPF ELF Object: xdp_filter.o ]
   ├── Section .text: Native BPF bytecode instructions
   ├── Section .maps: Map definitions (blocked_ip_map)
   └── Section .BTF : BPF Type Format metadata
                    │
                    ▼ libbpf / blackbox::XdpManager
 [ Kernel Verifier & JIT Compiler ]
                    │
                    ▼
 [ Native x86-64 / ARM64 Machine Instructions in Driver Ring ]
```

---

## 2. The Official Compilation Script (`bpf/build_bpf.sh`)

```bash
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/../build/bpf"
mkdir -p "${OUTPUT_DIR}"

CLANG="${CLANG:-clang-16}"
LLVM_STRIP="${LLVM_STRIP:-llvm-strip-16}"

# Detect host architecture for correct macro headers
ARCH=$(uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')

echo "[*] Compiling xdp_filter.c for architecture: ${ARCH} using ${CLANG}..."

${CLANG} -O2 -g \
    -target bpf \
    -D__TARGET_ARCH_${ARCH} \
    -I/usr/include \
    -I/usr/include/${uname_m:="$(uname -m)-linux-gnu"} \
    -Wall \
    -Wextra \
    -Werror \
    -c "${SCRIPT_DIR}/xdp_filter.c" \
    -o "${OUTPUT_DIR}/xdp_filter.o"

# Strip non-essential symbols while preserving BTF debug information
${LLVM_STRIP} -g "${OUTPUT_DIR}/xdp_filter.o"

echo "[+] Compilation successful: ${OUTPUT_DIR}/xdp_filter.o"
```

---

## 3. Disassembling and Auditing BPF Bytecode

Verify the compiled instructions using `llvm-objdump`:

```bash
llvm-objdump-16 -d build/bpf/xdp_filter.o
```

### Sample Disassembly:
```text
0000000000000000 <xdp_threat_filter>:
       0:       r2 = *(u32 *)(r1 + 0x4)
       1:       r1 = *(u32 *)(r1 + 0x0)
       2:       r3 = r1
       3:       r3 += 0xe
       4:       if r3 > r2 goto +0x1d <LBB0_7>
       5:       r4 = *(u16 *)(r1 + 0xc)
       6:       if r4 != 0x8 goto +0x1b <LBB0_7>
       7:       r3 = r1
       8:       r3 += 0x22
       9:       if r3 > r2 goto +0x18 <LBB0_7>
```

Lines 4 and 9 illustrate the compiler emitting the bounds-checking comparison instructions before any packet dereference.

