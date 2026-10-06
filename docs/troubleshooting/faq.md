# Technical Frequently Asked Questions (FAQ)

---

### Q1: How does `blackbox-essential` achieve $< 0.84\,\mu\text{s}$ drops while `iptables` takes milliseconds?
Linux `iptables` and `nftables` process packets in the network layer after the kernel allocates a 240-byte `struct sk_buff`, initializes protocol structures, and navigates kernel softirq routines. `blackbox-essential` runs an eBPF program directly inside the physical NIC driver's receive ring (XDP). Malicious packets are identified and discarded in hardware descriptor memory before any socket buffer memory is allocated, dropping packets within $\sim 118\text{ CPU cycles}$.

---

### Q2: Does `blackbox-essential` interfere with existing firewalls (`ufw`, `firewalld`)?
**No.** `blackbox-essential` functions as an upstream, in-kernel drop gate. Any packet that matches an active block rule in `blocked_ip_map` is dropped immediately (`XDP_DROP`). Packets deemed clean return `XDP_PASS` and proceed to standard host firewall rules (`ufw`, `nftables`, `iptables`) without modification.

---

### Q3: What happens if an edge hardware TPM 2.0 chip fails or burns out?
`blackbox::HardwareIdentity` implements an automated fallback hierarchy:
* If physical TPM 2.0 (`/dev/tpmrm0`) is absent or unresponsive, the engine downgrades to **Tier 3 (DMI Motherboard Hash)**.
* The daemon continues operating and enforcing kernel packet drops, but tags all emitted telemetry and audit logs with `TIER3_UNATTESTED_FLAG`.
* The central fleet command (`sentinel-nexus`) alerts operators to schedule hardware replacement while maintaining edge packet protection.

---

### Q4: Can `blocked_ip_map` store millions of IP addresses simultaneously?
By default, `blocked_ip_map` is pre-allocated with $65{,}536\text{ entries}$ ($\sim 2.1\text{ MB}$ of pinned kernel RAM) to guarantee sub-microsecond hash lookup performance. You can expand `max_blocked_ips` in `XdpConfig` up to several million entries, provided the host has sufficient pinned physical memory available:
$$\text{Memory Required} \approx \text{max\_entries} \times 32\,\text{bytes}$$
For $1{,}000{,}000\text{ IPs}$, the kernel map consumes $\approx 32\,\text{MB}$ of physical RAM.

---

### Q5: Does `blackbox-essential` run inside rootless Docker containers?
**No.** Attaching eBPF programs to physical network drivers and locking physical memory pages require Linux kernel administrative privileges:
* `CAP_NET_ADMIN`: Required to attach XDP programs to netdevs.
* `CAP_BPF` (or `CAP_SYS_ADMIN` on older kernels): Required to load BPF bytecode and create maps.
* `CAP_SYS_RESOURCE`: Required to set `ulimit -l` (unlimited memory locking).

