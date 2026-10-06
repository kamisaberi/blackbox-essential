# Verifying Kernel Support: eBPF JIT, BTF & BPF Filesystem

Before running `blackbox-essential` in production, verify that your host operating system has enabled the eBPF Just-In-Time (JIT) compiler, the BPF Type Format (BTF) subsystem, and the mounted BPF virtual filesystem (`bpffs`).

---

## 1. Checking eBPF JIT Status

The eBPF JIT compiler translates BPF bytecode instructions into native host machine code (x86-64 or ARM64) during program load. If JIT is disabled, the kernel falls back to an interpreter, increasing mitigation latency from $< 0.84\,\mu\text{s}$ to $> 12.0\,\mu\text{s}$.

Verify JIT status:

```bash
cat /proc/sys/net/core/bpf_jit_enable
```

* `1`: **Enabled** (Required for production).
* `2`: **Enabled with Debug Trace Mode**.
* `0`: **Disabled** (Unacceptable for low-latency mitigation).

To enable eBPF JIT permanently:

```bash
echo "net.core.bpf_jit_enable = 1" | sudo tee -a /etc/sysctl.d/99-bpf.conf
echo "net.core.bpf_jit_harden = 2" | sudo tee -a /etc/sysctl.d/99-bpf.conf
sudo sysctl --system
```

---

## 2. Verifying Kernel BTF Support (`/sys/kernel/btf/vmlinux`)

BPF Type Format (BTF) enables Compile Once – Run Everywhere (CO-RE), allowing eBPF programs to read internal kernel data structures reliably across kernel versions without re-compilation.

Check for the existence of kernel type information:

```bash
ls -l /sys/kernel/btf/vmlinux
```

If this file is missing, install the debug symbol package for your kernel:

```bash
sudo apt-get install -y linux-image-$(uname -r)-dbg
```

---

## 3. Mounting the BPF Virtual Filesystem (`bpffs`)

`blackbox-essential` pins BPF maps to persistent filesystem namespaces in `/sys/fs/bpf` so that user-space control daemons can read telemetry and insert blocked IPs without keeping file descriptors open continuously.

Check if `bpffs` is mounted:

```bash
mount | grep bpf
```

### Expected Output
```text
none on /sys/fs/bpf type bpf (rw,nosuid,nodev,noexec,relatime,mode=700)
```

If not mounted, mount it manually:

```bash
sudo mount -t bpf bpffs /sys/fs/bpf
```

To persist the mount across system reboots, add the following line to `/etc/fstab`:

```text
bpffs    /sys/fs/bpf    bpf    defaults    0    0
```

---

## 4. Querying XDP Driver Support on Network Interfaces

Query whether your network adapter supports native driver mode (`xdpdrv`):

```bash
ip link show eth0
```

To inspect eBPF capabilities using `bpftool`:

```bash
sudo bpftool feature probe
```

Verify that `Program types: xdp` and `Map types: hash` are marked as **available**.

