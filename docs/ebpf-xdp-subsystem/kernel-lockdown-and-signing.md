# Linux Kernel Lockdown Mode & Cryptographic Signing

In hardened enterprise and defense environments, operating systems enforce **Linux Kernel Lockdown Mode** (`integrity` or `confidentiality`). This security policy restricts kernel modifications and untrusted eBPF programs.

`blackbox-essential` complies with Kernel Lockdown policies through cryptographic program signing and Secure Boot integration.

---

## 1. Understanding Lockdown Modes

| Lockdown State | Impact on eBPF & XDP Programs | `blackbox-essential` Status |
| :--- | :--- | :--- |
| **`none`** | Full access to all BPF program types and helper calls. | Operational |
| **`integrity`** | BPF features that can modify kernel memory are disabled. | **Operational:** XDP and read-only tracing are permitted. |
| **`confidentiality`** | Inspecting kernel memory structures (e.g., kprobes) is blocked. | **Operational:** Packet filtering using XDP remains permitted. |

Check the active lockdown state:

```bash
cat /sys/kernel/security/lockdown
```

---

## 2. Cryptographic ELF Signing via `sign-file`

To satisfy enterprise Secure Boot and custom kernel integrity verification policies, the compiled BPF object (`xdp_filter.o`) is signed using the host's private X.509 certificate:

```bash
# Sign the compiled BPF ELF object
/usr/src/linux-headers-$(uname -r)/scripts/sign-file \
    sha256 \
    /etc/ssl/certs/blackbox_kernel_sign.key \
    /etc/ssl/certs/blackbox_kernel_sign.crt \
    build/bpf/xdp_filter.o
```

---

## 3. TPM 2.0 PCR 7 Secure Boot Measurement

When Secure Boot is enabled, the cryptographic signature of the loaded eBPF mitigation module is measured into TPM 2.0 Platform Configuration Register 7 (**PCR 7**). 

Any unauthorized modification of `xdp_filter.o` will:
1. Cause the kernel verification loader to reject the object (`-EKEYREJECTED`).
2. Invalidate the TPM 2.0 PCR 7 quote, preventing the daemon from authenticating with the central fleet controller (`sentinel-nexus`).
