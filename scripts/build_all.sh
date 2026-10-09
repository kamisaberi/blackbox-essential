#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "$PROJECT_ROOT"

echo "============================================================"
echo "    BLACKBOX-ESSENTIAL: MASTER CORE COMPILATION             "
echo "============================================================"

# 1. Compile eBPF CO-RE Kernel Bytecode (xdp_drop.o)
echo "[+] Compiling eBPF CO-RE Kernel bytecode..."
./scripts/build_ebpf.sh

# 2. Build CMake Shared Library (libblackbox.so) and Executables
echo "[+] Compiling libblackbox.so, daemon, and AF_XDP targets..."
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)" blackbox blackbox_daemon test_af_xdp test_af_xdp_live

# 3. Install to system library path
echo "[+] Installing libblackbox.so to /usr/local/lib..."
sudo make install
sudo ldconfig
cd "$PROJECT_ROOT"

echo "============================================================"
echo " [SUCCESS] Core engine & AF_XDP binaries built cleanly!     "
echo " Output binaries:                                           "
echo "   - build/libblackbox.so (installed to /usr/local/lib)     "
echo "   - build/blackbox_daemon                                  "
echo "   - build/test_af_xdp                                      "
echo "   - build/test_af_xdp_live                                 "
echo "============================================================"
