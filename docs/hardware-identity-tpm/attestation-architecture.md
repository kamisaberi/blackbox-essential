# Hardware Attestation Architecture: Why Software Identities Fail

In hostile or zero-trust deployment environments (such as remote substations, offshore wind turbines, or public cloud edge nodes), software-only credentials—such as API tokens, hardcoded UUIDs, or filesystem-stored TLS private keys—fail to provide secure device identity.

`blackbox-essential` binds each running node to a **cryptographic silicon root of trust** using a 3-tier adaptive identity architecture.

---

## 1. Vulnerabilities of Software-Only Identities

```text
CONVENTIONAL SOFTWARE IDENTITY (Vulnerable to Replication & Theft):
 ┌─────────────────────────────────────────────────────────────┐
 │ Storage: /etc/sentinel/client_cert.pem & private_key.pem    │
 └──────────────────────────────┬──────────────────────────────┘
                                │
       ┌────────────────────────┼────────────────────────┐
       ▼                        ▼                        ▼
 [ Memory Scraped by    ] [ Stolen via Offline ] [ Duplicated via VM  ]
 [ Malicious Process    ] [ Disk Mount / Exfil ] [ Snapshot / Cloning ]
```

* **Filesystem Theft:** An attacker with physical access can mount an edge gateway's eMMC or NVMe drive, clone the private keys, and instantiate spoofed rogue appliances.
* **Hypervisor Duplication:** Virtual appliances running on hypervisors (VMware ESXi, KVM) can be snapshotted and cloned, resulting in multiple active nodes sharing identical credentials.

---

## 2. The 3-Tier Adaptive Identity Hierarchy

`blackbox::HardwareIdentity` dynamically discovers platform hardware and selects the highest security tier available:

```text
                        ┌─────────────────────────────────────┐
                        │   Hardware Discovery & Valuation    │
                        └──────────────────┬──────────────────┘
                                           │
         ┌─────────────────────────────────┼─────────────────────────────────┐
         ▼                                 ▼                                 ▼
 ┌───────────────┐                 ┌───────────────┐                 ┌───────────────┐
 │    TIER 1     │                 │    TIER 2     │                 │    TIER 3     │
 │  Physical TPM │                 │  Virtual TPM  │                 │  DMI UUID /   │
 │   2.0 Silicon │                 │ (vTPM / swtpm)│                 │  Machine-ID   │
 └───────┬───────┘                 └───────┬───────┘                 └───────┬───────┘
         │                                 │                                 │
         ▼                                 ▼                                 ▼
 Non-extractable keys;             Cryptographically bounded         Software fallback for
 TCG TSS2 quotes;                  to hypervisor vCenter CA;         legacy DIN-rail IPCs;
 Sealed to PCR 0 & 4.              Snapshot clone detection.         Tagged with untrusted flag.
```

---

## 3. Cryptographic Invariants

1. **Non-Extractable Keys:** The private Endorsement Key (EK) and Attestation Identity Key (AIK) never leave the physical TPM silicon boundary in plaintext.
2. **Freshness via External Nonce:** Cryptographic quotes incorporate a 32-byte single-use cryptographic nonce issued by the fleet orchestrator (`sentinel-nexus`), eliminating replay attacks.
3. **State Sealing:** Model weights and local decryption keys are cryptographically sealed to specific Platform Configuration Register values (PCR 0 and PCR 4). If the bootloader or UEFI firmware is modified, the TPM refuses to unseal the secrets.

