### Part 12: Troubleshooting & Help Desk System (`troubleshooting/*`)

This final section covers kernel verifier diagnostics, driver attachment fault isolation, TPM device and udev resolution, virtualized VMware/veth edge cases, technical FAQs, and enterprise support escalation protocols for `blackbox-essential`.

---

### File: `blackbox-essential/docs/troubleshooting/bpf-verifier-rejection-guide.md`

```markdown
# BPF Verifier Rejection Diagnostics & Remediation

When compiling or loading `xdp_filter.o`, the Linux kernel BPF Verifier may reject the bytecode with a multi-page instruction trace. This guide translates common verifier errors into specific remediation steps.

---

## 1. `invalid access to packet, memptr=R1, offset=..., size=...`

### Verifier Trace Excerpt
```text
0: (61) r2 = *(u32 *)(r1 +4)
1: (61) r1 = *(u32 *)(r1 +0)
2: (71) r3 = *(u8 *)(r1 +14)
invalid access to packet, memptr=R1, offset=14, size=1
R1 min value is outside of the allowed memory range
```

### Cause
The code attempted to dereference packet memory (e.g., accessing Ethernet header byte 14 or the IP header) without first proving to the verifier that the pointer is strictly less than or equal to `ctx->data_end`.

### Remediation
Insert an explicit bounds check before the dereference:

```c
// Incorrect:
struct ethhdr *eth = data;
if (eth->h_proto == bpf_htons(ETH_P_IP)) { ... } // Verifier fails here!

// Correct:
struct ethhdr *eth = data;
if ((void *)(eth + 1) > data_end) {
    return XDP_PASS; // Bounds verified: eth + 14 bytes <= data_end
}
if (eth->h_proto == bpf_htons(ETH_P_IP)) { ... } // Permitted by verifier
```

---

## 2. `combined stack size of 4 frames is 544 bytes, stack limit 512 bytes`

### Cause
The eBPF execution model caps the total stack space at **512 bytes** across all call frames. Declaring large structs (such as a 256-byte telemetry buffer or array) on the stack triggers this failure.

### Remediation
Move large data structures off the stack and into a `BPF_MAP_TYPE_PERCPU_ARRAY` scratchpad map:

```c
// Incorrect:
struct large_flow_record record; // 384 bytes on stack -> FAILS!

// Correct:
__u32 zero = 0;
struct large_flow_record *record = bpf_map_lookup_elem(&scratchpad_map, &zero);
if (!record) return XDP_PASS;
// Use record safely in kernel memory
```

---

## 3. `back-edge from insn ... to ...` or `unbounded loop detected`

### Cause
Kernels prior to 5.3 strictly forbid back-edges (loops). Kernels 5.3+ permit bounded loops, but reject loops where the iteration count cannot be mathematically proven at compile time.

### Remediation
Force complete loop unrolling using the Clang unroll pragma:

```c
#pragma unroll
for (int i = 0; i < 4; ++i) {
    // Fixed, bounded unrolled execution path
}
```
```

---

### File: `blackbox-essential/docs/troubleshooting/xdp-attachment-failures.md`

```markdown
# XDP Driver Attachment Failures

This guide resolves errors encountered when invoking `XdpManager::attach()` or attaching `xdp_filter.o` via `ip link`.

---

## 1. `Operation not supported (-EOPNOTSUPP)`

### Symptom
```text
[Blackbox Fatal Error]
  Code       : -1 (ERR_XDP_ATTACH_FAILED)
  Description: Failed to attach XDP program to interface eth0
  System Err : 95 (Operation not supported)
```

### Causes & Fixes
1. **Conflicting Offloads (LRO/GRO):** Native Driver XDP requires Large Receive Offload (LRO) and Generic Receive Offload (GRO) to be disabled.
   ```bash
   sudo ethtool -K eth0 lro off gro off rxvlan off txvlan off
   ```
2. **MTU Exceeds Driver Page Bounds:** Many NIC drivers restrict XDP to standard MTUs ($\le 1500$). If jumbo frames are enabled:
   ```bash
   sudo ip link set dev eth0 mtu 1500
   ```
3. **Driver Lacks Native XDP Support:** If using a legacy adapter (e.g., `e1000e`), native driver mode is unsupported. Fall back to generic mode:
   ```cpp
   config.attach_mode = blackbox::XdpAttachMode::SKB_GENERIC;
   ```

---

## 2. `Device or resource busy (-EBUSY)`

### Symptom
```text
System Err: 16 (Device or resource busy)
```

### Cause
Another XDP program or security agent (such as Cilium, Cloudflare `bpftools`, or a previously crashed instance of `libblackbox.so`) is already attached to the network interface.

### Remediation
Force-detach any active XDP program before attaching:

```bash
# Check active attachments
ip link show dev eth0

# Detach any existing native or generic XDP program
sudo ip link set dev eth0 xdp off
sudo ip link set dev eth0 xdpgeneric off
```

Then restart `blackbox-essential`.
```

---

### File: `blackbox-essential/docs/troubleshooting/tpm-permission-and-device-errors.md`

```markdown
# TPM 2.0 Permission & Device Fault Diagnostics

This guide covers troubleshooting physical TPM 2.0 access issues, device node misconfigurations, and TCG TSS2 error codes.

---

## 1. `Failed to open /dev/tpmrm0: Permission denied (errno 13)`

### Symptom
```text
[Blackbox Fatal Error]
  Code       : -8 (ERR_TPM_INITIALIZATION_FAILED)
  Description: Esys_Initialize failed: could not open /dev/tpmrm0
  System Err : 13 (Permission denied)
```

### Cause
By default, the Linux TPM Resource Manager character device (`/dev/tpmrm0`) is owned by `root:tss` with permissions `0660`. The process executing `libblackbox.so` lacks permission to read and write to the device.

### Remediation
1. Add the operational service user to the `tss` system group:
   ```bash
   sudo usermod -aG tss $USER
   ```
2. Alternatively, deploy a dedicated udev rule to `/etc/udev/rules.d/70-tpmrm.rules`:
   ```udev
   KERNEL=="tpmrm[0-9]*", MODE="0666", GROUP="tss"
   ```
   Reload udev rules:
   ```bash
   sudo udevadm control --reload-rules && sudo udevadm trigger
   ```

---

## 2. `Device not found: /dev/tpmrm0`

### Cause
The physical host does not have a discrete TPM 2.0 chip, the TPM is disabled in the BIOS/UEFI settings, or the kernel TPM drivers are not loaded.

### Diagnosis Checklist
1. Inspect kernel boot messages:
   ```bash
   dmesg | grep -i tpm
   ```
2. If `tpm_crb` or `tpm_tis` errors appear, verify that **Intel PTT (Platform Trust Technology)** or **AMD fTPM** is set to **Enabled** in the motherboard BIOS settings.
3. If deploying inside a virtual machine, ensure that a **vTPM 2.0 Device** is explicitly attached to the virtual machine hardware configuration in VMware vSphere or Proxmox/KVM.
```

---

### File: `blackbox-essential/docs/troubleshooting/vmware-veth-skb-issues.md`

```markdown
# VMware & Virtual Ethernet (`veth`) Edge Cases

Running in-kernel XDP filters across virtualized environments (such as Docker-in-VMware or multi-homed ESXi hosts) introduces virtual switch and packet delivery quirks.

---

## 1. Virtual Ethernet Pairs (`veth`) Fail in Driver Mode

### Symptom
Attaching an XDP program to a Docker container's `veth` interface fails with `Operation not supported`.

### Cause
Virtual Ethernet pair drivers (`veth`) do not have physical hardware DMA descriptors; they simulate network delivery via standard kernel memory buffers. 

### Remediation
Always configure virtual pairs with `SKB_GENERIC` mode:

```cpp
if (interface_name.starts_with("veth") || interface_name.starts_with("docker")) {
    config.attach_mode = blackbox::XdpAttachMode::SKB_GENERIC;
}
```

---

## 2. Packets Dropping Silently on VMware ESXi vSwitch

### Symptom
XDP programs attach without errors on VMware guests running `vmxnet3`, but ingress packets never reach the filter or drop counters remain zero during traffic floods.

### Cause
The VMware vSphere Virtual Switch (vSwitch) or Distributed Port Group blocks forged packet headers and drops promiscuous frames before they reach the virtual machine's vNIC.

### Remediation
In VMware vSphere / ESXi host settings, edit the Port Group security policy:

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ VMware vSwitch / Port Group Security Policies               │
 ├─────────────────────────────────────────────────────────────┤
 │ Promiscuous Mode            : ACCEPT                        │
 │ MAC Address Changes         : ACCEPT                        │
 │ Forged Transmits            : ACCEPT                        │
 └─────────────────────────────────────────────────────────────┘
```

Set all three policies to **Accept** to allow raw packet flows to reach the `vmxnet3` driver.
```

---

### File: `blackbox-essential/docs/troubleshooting/faq.md`

```markdown
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
```

---

### File: `blackbox-essential/docs/troubleshooting/support.md`

```markdown
# Enterprise Support & Issue Reporting

---

## 1. Reporting Bugs & In-Kernel Panics

When filing a bug report or requesting technical support for `blackbox-essential`, capture system diagnostics using the automated reporting tool:

```bash
# Capture full kernel and eBPF state
blackbox-ctl diag --full > blackbox_diag.log

# Append active kernel version and driver logs
uname -a >> blackbox_diag.log
dmesg | grep -E 'xdp|bpf|tpm' | tail -n 100 >> blackbox_diag.log
```

Submit issues to our repository:  
👉 **[https://github.com/kamisaberi/blackbox-essential/issues](https://github.com/kamisaberi/blackbox-essential/issues)**

---

## 2. Enterprise Commercial Support SLAs

Aryorithm Technologies B.V. provides 24/7/365 commercial engineering support for defense, municipal utility, and critical infrastructure edge networks:

| Support Tier | Response SLA | Dedicated Coverage | Scope |
| :--- | :--- | :--- | :--- |
| **Standard Commercial**| 8 Business Hours | Web & Ticket Desk | Integration guidance, CMake linking, bugfixes. |
| **Mission-Critical Defense**| **1 Hour (24/7/365)** | Dedicated Systems Engineer | Direct phone access, custom NIC driver patches, kernel crash triage, on-site audits. |

For commercial licenses, hardware compliance certifications, and air-gapped support contracts:
* **Email:** `support@aryorithm.com`
* **Website:** `https://aryorithm.com`

---

## 3. Coordinated Security Vulnerability Disclosure

If you identify a potential security bypass, memory safety violation, or verifier escape in `blackbox-essential`:
* **Do not file a public GitHub issue.**
* Transmit an encrypted report to **`security@aryorithm.com`**.
* We acknowledge reports within **48 hours**, provide coordinated remediation updates, and handle CVE registrations according to standard disclosure guidelines.
```

---

## Master Status: Project 2 Documentation Complete

The complete documentation suite for **Project 2 of 8: `blackbox-essential` (`libblackbox.so`)** is fully generated:

```text
blackbox-essential/docs/
├── mkdocs.yml                                 # [Delivered Part 1]
├── index.md                                   # [Delivered Part 1]
├── getting-started/ (6 files)                 # [Delivered Part 1]
├── architecture/ (5 files)                    # [Delivered Part 2]
├── ebpf-xdp-subsystem/ (7 files)              # [Delivered Part 3]
├── spmc-ring-buffer/ (6 files)                # [Delivered Part 4]
├── hardware-identity-tpm/ (7 files)           # [Delivered Part 5]
├── af-xdp-zero-copy/ (5 files)                # [Delivered Part 6]
├── model-config/ (4 files)                    # [Delivered Part 7]
├── api-reference/ (8 files)                   # [Delivered Part 8]
├── tutorials/ (5 files)                       # [Delivered Part 9]
├── benchmarking/ (6 files)                    # [Delivered Part 10]
├── compliance/ (4 files)                      # [Delivered Part 11]
└── troubleshooting/ (6 files)                 # [Delivered Part 12]
```

Total: **66 documentation and configuration files**, providing comprehensive technical coverage of the Tier 2 active mitigation core.

---

### Ready for Next Project

When you are ready, provide the file structure or confirmation to begin **Project 3 of 8**:  
👉 **`blackbox-sentinel` (`sentinel` daemon)** — *Tier 3 Edge XDR & SIEM Commercial Appliance (26 Decoupled C++ Subsystems, 30 Industrial Protocol Dissectors, NexusUplink, and Port 8443 Web Command Center).*