# Getting Started

Welcome to the **Blackbox Essential (`libblackbox.so`)** Getting Started guide. This section provides step-by-step documentation for configuring, compiling, and validating the Tier 2 active mitigation core.

## Section Navigation

- [Mission & Overview](./overview.md): High-level operational invariants, sub-microsecond SLA, and the 3 architectural pillars.
- [System Requirements](./system-requirements.md): Operating system specifications, kernel BPF flags, and compiler standards.
- [Installation](./installation.md): Compiling `xdp_filter.o` and `libblackbox.so` using CMake and Ninja.
- [CMake Integration](./cmake-integration.md): Linking `libblackbox.so` into external C++20 applications via `find_package` or `FetchContent`.
- [Quickstart Drop Test](./quickstart-drop-test.md): 5-minute walkthrough compiling and verifying in-kernel packet drops on `lo` or physical interfaces.
- [Verifying Kernel Support](./verifying-kernel-support.md): Validating eBPF JIT compiler status, BTF debug symbols, and `bpffs` filesystem mounts.
