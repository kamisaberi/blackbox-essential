#!/usr/bin/env bash
set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}=====================================================${NC}"
echo -e "${GREEN}      Blackbox Autonomous Security Installer        ${NC}"
echo -e "${GREEN}=====================================================${NC}"

if [ "$EUID" -ne 0 ]; then
  echo -e "${YELLOW}Please run as root / sudo to install system dependencies.${NC}"
  exit 1
fi

echo "Updating APT package lists..."
apt-get update

echo "Installing Linux build tools, eBPF libraries, and SQLite3..."
apt-get install -y \
    build-essential \
    cmake \
    clang \
    llvm \
    libbpf-dev \
    libelf-dev \
    libsqlite3-dev \
    sqlite3 \
    iptables \
    net-tools \
    pcaputils

echo -e "${GREEN}Checking for xInfer Essential Engine (libxinfer.so)...${NC}"
if [ -f "/usr/local/lib/libxinfer.so" ] || [ -f "/usr/lib/libxinfer.so" ]; then
    echo -e "${GREEN}libxinfer.so detected!${NC}"
else
    echo -e "${YELLOW}Warning: libxinfer.so not found in standard library paths.${NC}"
    echo "Please ensure xinfer-essential is compiled and installed (sudo make install)."
fi

ldconfig
echo -e "${GREEN}Blackbox dependencies installed successfully!${NC}"