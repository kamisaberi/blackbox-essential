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