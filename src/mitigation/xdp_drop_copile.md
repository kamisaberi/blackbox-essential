```
clang -O2 -g -target bpf -D__TARGET_ARCH_x86 \
  -I/home/kami/blackbox-essential/src/mitigation \
  -I/usr/include/$(uname -m)-linux-gnu \
  -c /home/kami/blackbox-essential/src/mitigation/xdp_drop.c \
  -o /home/kami/blackbox-essential/src/mitigation/xdp_drop.o

  ```