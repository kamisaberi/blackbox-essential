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

