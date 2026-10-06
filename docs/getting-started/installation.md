# Building & Installing `blackbox-essential`

This guide explains how to compile the core C++20 shared library (`libblackbox.so`), compile the eBPF kernel program (`xdp_filter.o`), and install headers and libraries system-wide.

---

## 1. Clone the Codebase

```bash
git clone --recurse-submodules https://github.com/kamisaberi/blackbox-essential.git
cd blackbox-essential
```

---

## 2. Compile In-Kernel eBPF Bytecode

Before compiling the C++ library, compile the C eBPF kernel filter to produce the BPF object file:

```bash
cd bpf
chmod +x build_bpf.sh
./build_bpf.sh
```

### What `build_bpf.sh` Executes:

```bash
clang-16 -O2 -g -target bpf \
    -D__TARGET_ARCH_x86 \
    -I/usr/include/$(uname -m)-linux-gnu \
    -c xdp_filter.c -o xdp_filter.o
```

Verify that the BPF object was generated successfully:

```bash
llvm-objdump-16 -h xdp_filter.o
```

You should see sections for `.text`, `maps`, and `.BTF`.

---

## 3. Configure and Compile `libblackbox.so`

Navigate to the project root and configure the build directory using CMake:

```bash
cd ..
mkdir build && cd build

cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=clang++-16 \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBLACKBOX_BUILD_TESTS=ON \
    -DBLACKBOX_ENABLE_TPM=ON ..
```

### Common CMake Configuration Flags

| Option | Default | Description |
| :--- | :--- | :--- |
| `CMAKE_BUILD_TYPE` | `Release` | Build mode (`Release`, `Debug`, `RelWithDebInfo`). |
| `BLACKBOX_ENABLE_TPM` | `ON` | Compiles physical TPM 2.0 TSS2 hardware attestation hooks. |
| `BLACKBOX_BUILD_TESTS`| `ON` | Compiles unit tests and benchmark harnesses. |
| `BLACKBOX_BUILD_CLI` | `ON` | Builds the standalone CLI utility (`blackbox-ctl`). |

Execute the build:

```bash
ninja -j$(nproc)
```

---

## 4. Install System-Wide

Install binaries, dynamic libraries, and development headers to `/usr/local`:

```bash
sudo ninja install
sudo ldconfig
```

### Installed File Hierarchy

```text
/usr/local/
├── include/
│   └── blackbox/
│       ├── blackbox.hpp
│       ├── xdp_manager.hpp
│       ├── event_ring_buffer.hpp
│       ├── hardware_identity.hpp
│       └── model_config.hpp
├── lib/
│   ├── libblackbox.so -> libblackbox.so.1.0.0
│   ├── libblackbox.so.1
│   ├── libblackbox.so.1.0.0
│   └── bpf/
│       └── xdp_filter.o
└── bin/
    └── blackbox-ctl
```

