# System Requirements & Prerequisites

Review the toolchain, kernel configuration, and driver dependencies before building and running `blackbox-essential`.

---

## 1. Operating System & Kernel Version

`blackbox-essential` requires a modern 64-bit Linux kernel with native eBPF, BTF (BPF Type Format), and XDP driver support.

| Operating System | Minimum Kernel | Recommended Kernel | Target Architectures |
| :--- | :--- | :--- | :--- |
| **Ubuntu Linux** | 22.04 LTS (Kernel 5.15) | 24.04 / 26.04 (Kernel 6.8+) | `x86_64`, `aarch64` |
| **Debian** | 12 (Bookworm - 6.1) | 12 (Kernel 6.1+) | `x86_64`, `aarch64` |
| **Red Hat Enterprise Linux** | RHEL 9.0 (Kernel 5.14) | RHEL 9.4 (Kernel 5.14+) | `x86_64` |

---

## 2. Required Linux Kernel Configuration Flags

Ensure the host kernel was compiled with the following BPF features enabled:

```text
CONFIG_BPF=y
CONFIG_BPF_SYSCALL=y
CONFIG_BPF_JIT=y
CONFIG_HAVE_EBPF_JIT=y
CONFIG_BPF_EVENTS=y
CONFIG_NET_CLS_ACT=y
CONFIG_XDP_SOCKETS=y
CONFIG_DEBUG_INFO_BTF=y
```

### Verifying Active Kernel Flags

```bash
# Check BPF configuration from running kernel
cat /boot/config-$(uname -r) | grep -E 'CONFIG_BPF|CONFIG_XDP'
```

---

## 3. Compiler & System Packages

`blackbox-essential` requires an ISO C++20 compiler for user-space control code and Clang/LLVM for compiling in-kernel eBPF bytecode:

* **Host C++20 Compiler:** GCC 12.1+ or Clang 16.0+
* **eBPF Compiler:** Clang/LLVM 15.0+ (Clang 16/18 recommended)
* **Kernel Development Headers:** `linux-headers-$(uname -r)`
* **System Libraries:**
  * `libelf-dev`: ELF binary and symbol table parsing.
  * `libz-dev`: Compression support for kernel debug symbols.
  * `libtss2-dev`: TCG TPM 2.0 Software Stack (TSS2) for hardware attestation.

### Ubuntu 24.04 / 22.04 LTS Package Installation

```bash
sudo apt-get update && sudo apt-get install -y \
    build-essential \
    clang-16 \
    llvm-16 \
    lld-16 \
    libelf-dev \
    zlib1g-dev \
    libbpf-dev \
    linux-headers-$(uname -r) \
    libtss2-dev \
    tpm2-tools \
    cmake \
    ninja-build
```

---

## 4. Supported Network Hardware & Driver Modes

To achieve the sub-microsecond mitigation SLA, the target network interface card (NIC) should support Native Driver XDP:

| NIC Hardware Architecture | Driver Name | XDP Native Mode Support | Multi-Queue RSS | Max Throughput |
| :--- | :--- | :--- | :--- | :--- |
| **Intel 10GbE / 40GbE** | `ixgbe`, `i40e`, `ice` | Full Driver Mode | Yes (Up to 64 queues) | $14.8\text{ Mpps}$ |
| **Mellanox ConnectX-4/5/6**| `mlx5_core` | Full Driver Mode | Yes (Up to 128 queues)| $25.0\text{ Mpps}$ |
| **VMware Virtual NIC** | `vmxnet3` | Full Driver Mode | Yes (Up to 8 queues)  | $2.5\text{ Mpps}$ |
| **Virtual Ethernet Pairs** | `veth` | Generic / Driver Mode| Yes | $1.8\text{ Mpps}$ |

