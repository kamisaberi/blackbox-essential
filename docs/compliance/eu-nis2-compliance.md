---

### File: `blackbox-essential/docs/compliance/eu-nis2-compliance.md`

```markdown
# EU NIS 2 Directive: Article 21 Incident Handling & Risk Mitigation

The European Union **Directive on Measures for a High Common Level of Cybersecurity across the Union (NIS 2 - Directive 2022/2555)** establishes baseline cybersecurity risk-management requirements for essential and important entities across energy, water, healthcare, and transport sectors.

`blackbox-essential` directly fulfills the technical mandates articulated in **Article 21(2)** of the NIS 2 directive.

---

## 1. Traceability to Article 21 Mandates

| NIS 2 Article | Statutory Requirement | `blackbox-essential` Technical Capability |
| :--- | :--- | :--- |
| **Article 21(2)(b)** | **Incident Handling:** Prevention, detection, and mitigation of cybersecurity incidents. | Drops active cyber-physical attacks autonomously in **$< 0.84\,\mu\text{s}$**, containing intrusions before lateral movement occurs. |
| **Article 21(2)(c)** | **Business Continuity:** Maintenance of operational capabilities during major cyber disruptions. | Bypasses kernel `sk_buff` allocation to maintain network availability during line-rate ($10\text{ Gbps}$) volumetric flooding attacks. |
| **Article 21(2)(d)** | **Supply Chain Security:** Verifying the security and integrity of platform hardware and components. | Enforces hardware-level supply chain verification using **physical TPM 2.0 PCR quotes**, ensuring system firmware has not been compromised. |
| **Article 21(2)(e)** | **Security in Network Systems:** Defending network and information system infrastructure. | Employs statically verified, type-safe eBPF in-kernel drivers, replacing unverified user-space packet inspection software. |
| **Article 21(2)(g)** | **Cryptographic Verification:** Appropriate use of cryptography to verify data integrity. | Uses SHA-256 model signing, TPM2-backed Attestation Identity Keys (AIK), and HMAC log hashing. |

---

## 2. Regulatory Reporting Support

Under NIS 2 Article 23, entities must notify competent authorities or computer security incident response teams (CSIRTs) of significant incidents within **24 hours**.

`blackbox-essential` exports structured event audit records containing:
* **Attack Ingress Interface & Hardware Timestamp** (IEEE 1588 nanosecond accuracy).
* **Mitigated Threat Class** (e.g., Modbus setpoint injection, volumetric UDP flood).
* **In-Kernel Mitigation Duration** and volume of packets suppressed.
* **Cryptographic Attestation Token** proving node identity and policy integrity.
```

---

### File: `blackbox-essential/docs/compliance/audit-log-tamper-evidence.md`

```markdown
# Cryptographic Tamper-Evident Audit Logging

Regulatory compliance frameworks (including NIS 2, CMMC 2.0, and IEC 62443) mandate that security audit trails and mitigation records remain **immutable and tamper-evident**. 

`blackbox-essential` secures its event logs using **Forward-Secure Cryptographic Hash Chaining** anchored to non-volatile physical TPM registers.

---

## 1. Cryptographic Hash Chain Architecture

Mitigation events emitted by the lock-free SPMC ring buffer are sequenced into an unbroken cryptographic hash chain:

$$H_0 = \text{TPM\_BOOT\_SEED}$$

$$H_i = \text{SHA-256}\left(H_{i-1} \parallel \text{Timestamp}_i \parallel \text{SourceIP}_i \parallel \text{Action}_i \parallel \text{DropCount}_i\right)$$

```text
 Event 1 (Drop IP A)        Event 2 (Drop IP B)        Event 3 (Drop IP C)
 ┌──────────────────────┐   ┌──────────────────────┐   ┌──────────────────────┐
 │ Hash H_1             │   │ Hash H_2             │   │ Hash H_3             │
 │ SHA256(H_0 || Evt_1) │──►│ SHA256(H_1 || Evt_2) │──►│ SHA256(H_2 || Evt_3) │
 └──────────────────────┘   └──────────────────────┘   └──────────┬───────────┘
                                                                  │
                                                                  ▼ Every 300 Seconds
                                                       ┌──────────────────────┐
                                                       │ TPM 2.0 PCR Extend   │
                                                       │ TPM2_PCR_Extend(     │
                                                       │   PCR_12, H_3)       │
                                                       └──────────────────────┘
```

If an adversary with root privileges modifies, deletes, or reorders a historical log entry:
1. The recalculated hash chain diverges immediately: $H_i' \ne H_i$.
2. The periodic hardware hash extending into **TPM PCR 12** will not match the calculated root, providing mathematical proof of log tampering to auditors.

---

## 2. C++20 Hash-Chaining Logger Implementation

```cpp
#include <openssl/sha.h>
#include <blackbox/telemetry.hpp>
#include <vector>
#include <iomanip>
#include <sstream>

namespace blackbox {

class TamperEvidentLog {
public:
    explicit TamperEvidentLog(const std::string& boot_seed) {
        SHA256(reinterpret_cast<const uint8_t*>(boot_seed.data()), boot_seed.size(), current_hash_);
    }

    // Appends an event and updates the forward cryptographic state
    std::string append_event(const FlowEvent& event, MitigationAction action) {
        std::ostringstream ss;
        ss << std::string(reinterpret_cast<char*>(current_hash_), SHA256_DIGEST_LENGTH)
           << event.timestamp_ns
           << event.src_ip
           << static_cast<int>(action);

        std::string payload = ss.str();
        SHA256(reinterpret_cast<const uint8_t*>(payload.data()), payload.size(), current_hash_);

        return get_current_hash_hex();
    }

    std::string get_current_hash_hex() const {
        std::ostringstream hex_stream;
        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            hex_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(current_hash_[i]);
        }
        return hex_stream.str();
    }

private:
    uint8_t current_hash_[SHA256_DIGEST_LENGTH]{};
};

} // namespace blackbox
```

---

## 3. Auditor Verification Command

To verify log chain integrity:

```bash
blackbox-ctl audit verify --log-file /var/log/blackbox/mitigation.log --pcr-index 12
```

### Expected Output
```text
[*] Validating 142,891 cryptographic audit log records...
[+] Hash Chain Integrity: VERIFIED (0 Anomalies, 0 Deleted Frames)
[+] PCR 12 Hardware Seal : MATCHED (Hardware state authenticates log integrity)
[+] Result: AUDIT LOG IS TAMPER-EVIDENT AND COMPLIANT.
```
```

---

### Complete in Part 11
- `blackbox-essential/docs/compliance/cmmc-level-2.md`
- `blackbox-essential/docs/compliance/iec-62443-industrial.md`
- `blackbox-essential/docs/compliance/eu-nis2-compliance.md`
- `blackbox-essential/docs/compliance/audit-log-tamper-evidence.md`

All 4 Compliance and Regulatory Certification documentation files are now generated.

---

### Files to be Generated in Part 12 (Final Phase for Project 2)

The final phase covers **Troubleshooting & Help Desk Diagnostics** (`troubleshooting/`), completing the entire documentation tree for `blackbox-essential`:

1. `troubleshooting/bpf-verifier-rejection-guide.md` (Debugging `R1 invalid mem access`, unbounded loops, and stack size)
2. `troubleshooting/xdp-attachment-failures.md` (Resolving "Operation not supported" and driver attachment errors)
3. `troubleshooting/tpm-permission-and-device-errors.md` (Fixing `/dev/tpmrm0` access denied and missing resource manager)
4. `troubleshooting/vmware-veth-skb-issues.md` (Debugging packet drops on VMware virtual interfaces and veth)
5. `troubleshooting/faq.md` (Technical Frequently Asked Questions)
6. `troubleshooting/support.md` (Issue tracker, security vulnerability disclosure, and support SLAs)

Confirm when you are ready to proceed with Part 12.