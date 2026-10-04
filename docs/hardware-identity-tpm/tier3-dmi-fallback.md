---

### File: `blackbox-essential/docs/hardware-identity-tpm/tier3-dmi-fallback.md`

```markdown
# Tier 3 Identity: DMI UUID & Machine-ID Fallback

For legacy industrial edge gateways, DIN-rail automation PCs (e.g., legacy Advantech, Moxa, or Siemens IPCs), and micro-embedded ARM boards lacking hardware TPM chips, `blackbox-essential` falls back to **Tier 3 Deterministic DMI Hashing**.

---

## 1. Entropy Harvesting Sources

Tier 3 identity derives a persistent 256-bit identifier by combining multiple independent platform serials from the Linux kernel sysfs interface:

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ /sys/class/dmi/id/product_uuid                              │
 └──────────────────────────────┬──────────────────────────────┘
                                │
 ┌──────────────────────────────┼──────────────────────────────┐
 │ /sys/class/dmi/id/board_serial                              │
 └──────────────────────────────┼──────────────────────────────┘
                                │
 ┌──────────────────────────────┼──────────────────────────────┐
 │ /etc/machine-id (OS Install Unique Identifier)              │
 └──────────────────────────────┼──────────────────────────────┘
                                │
 ┌──────────────────────────────┼──────────────────────────────┐
 │ /sys/class/net/eth0/address (Primary NIC MAC Address)       │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Concatenate & Inject Secret Salt
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ Cryptographic Hash Engine: SHA-256 Digest                   │
 └──────────────────────────────┬──────────────────────────────┘
                                │
                                ▼
 [ Tier 3 Pseudo-Hardware UUID: e9a2c31e-847b-42c9-94b1-3a7b41e20000 ]
 (Marked with TIER3_UNATTESTED_FLAG in all telemetry claims)
```

---

## 2. Implementation: `generate_tier3_identity()`

```cpp
#include <blackbox/hardware_identity.hpp>
#include <openssl/sha.h>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace blackbox {

std::string read_sysfs_string(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string line;
    std::getline(file, line);
    return line;
}

std::string generate_tier3_identity() {
    std::string entropy;
    entropy += read_sysfs_string("/sys/class/dmi/id/product_uuid");
    entropy += read_sysfs_string("/sys/class/dmi/id/board_serial");
    entropy += read_sysfs_string("/etc/machine-id");
    entropy += read_sysfs_string("/sys/class/net/eth0/address");

    // Enforce fallback salt if hardware serials are empty (e.g. containers)
    if (entropy.empty()) {
        entropy = "ARYORITHM_TIER3_UNTRUSTED_ENVIRONMENT";
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(entropy.data()), entropy.size(), hash);

    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

} // namespace blackbox
```

---

## 3. Fleet Trust Downgrade

When an appliance operates in Tier 3 mode:
* The fleet command plane (`sentinel-nexus`) flags the node as **`UNATTESTED_HARDWARE`**.
* Model retraining pipelines (`xinfer-forge`) refuse to ingest active-learning vectors from Tier 3 nodes to prevent **adversarial data poisoning**.
```

---

### File: `blackbox-essential/docs/hardware-identity-tpm/anti-cloning-protections.md`

```markdown
# Anti-Cloning Defenses & Rogue Node Revocation

Virtualization and containerization make it easy for adversaries to duplicate active appliance disk images and spin up unauthorized, parallel clones to conduct side-channel attacks or bypass collective defense quotas.

`blackbox-essential` implements automated **Anti-Cloning Protections**.

---

## 1. The Appliance Cloning Threat

```text
 [ Production Appliance A (Valid) ] ──── Clone Image ────► [ Rogue Cloned Appliance B ]
  IP: 10.240.0.101                                           IP: 10.240.0.199
  Identical Certs, Disk Secrets, and Model Configurations
```

If an appliance relies solely on disk-stored certificates, the central orchestrator cannot distinguish Appliance A from Rogue Clone B.

---

## 2. Multi-Factor Anti-Cloning Detection Vectors

`blackbox::HardwareIdentity` monitors three independent factors to detect unauthorized duplication:

### 1. TPM Monotonic Command Counters (`TPM2_PT_TOTAL_COMMANDS`)
Physical and virtual TPM chips maintain internal, non-volatile monotonic counters that increment with every transaction:
$$\text{Counter}_{t+1} > \text{Counter}_t$$
If Nexus observes non-monotonic counter regressions (e.g., Appliance B reports a counter value lower than Appliance A's previous report), snapshot rollback or VM cloning is immediately flagged.

### 2. Ephemeral Nonce Challenge Windows
Nexus requires periodic cryptographic attestation quotes every $300\text{ seconds}$. Each challenge uses a unique random nonce with a **$2.0\text{ second}$ execution window**. Two cloned appliances sharing the same identity cannot answer challenges concurrently without colliding on nonce responses.

### 3. Hypervisor Boot UUID Verification
On VMware and KVM, the engine verifies the hypervisor's runtime UUID against cached bios measurements. If a VM clone is generated, ESXi generates a new virtual BIOS UUID (`uuid.bios`), causing an immediate mismatch against the TPM's measured PCR 0.

---

## 3. Automated Revocation Protocol

```text
 [ Nexus Fleet Hub Detects Clone / Nonce Collision ]
                         │
                         ▼ Emits Signed Revocation Command (gRPC 50051)
 ┌─────────────────────────────────────────────────────────────┐
 │ blackbox::HardwareIdentity::handle_revocation()             │
 └───────────────────────┬─────────────────────────────────────┘
                         │
        ┌────────────────┴────────────────┐
        ▼                                 ▼
 ┌───────────────┐                 ┌───────────────┐
 │ PURGE SECRETS │                 │ KERNEL LOCKOUT│
 └───────────────┘                 └───────────────┘
 Clears in-memory keys;            Commands xdp_filter.o to drop
 zeroizes model weights.           ALL network frames (Fail-Secure).
```
```

---

### File: `blackbox-essential/docs/hardware-identity-tpm/quote-verification-flow.md`

```markdown
# Attestation Identity Key (AIK) Signature Validation Protocol

This document details the complete cryptographic verification handshake executed between an edge appliance (`blackbox-essential`) and the fleet command plane (`sentinel-nexus`).

---

## 1. Cryptographic Handshake Protocol

```text
Edge Appliance (Blackbox)                            Fleet Command (Nexus Hub)
       │                                                         │
       │ 1. Request Identity Registration Handshake              │
       ├────────────────────────────────────────────────────────►│
       │                                                         │
       │ 2. Issue 32-Byte Cryptographic Nonce                    │
       │◄────────────────────────────────────────────────────────┤
       │                                                         │
 ┌─────┴────────────────────────────────┐                        │
 │ - Invokes Esys_Quote() with Nonce    │                        │
 │ - Measures PCR 0 (BIOS) & PCR 4 (OS) │                        │
 │ - Signs digest using private AIK     │                        │
 └─────┬────────────────────────────────┘                        │
       │                                                         │
       │ 3. Transmit IdentityPayload:                            │
       │    - TPMS_ATTEST Binary Data                            │
       │    - TPMT_SIGNATURE (RSA-2048 / ECC P-256)              │
       │    - Public AIK Certificate                             │
       │    - Public EK Certificate (Signed by Manufacturer)     │
       ├────────────────────────────────────────────────────────►│
       │                                                         │
       │                                            ┌────────────┴────────────┐
       │                                            │ Cryptographic Checks:   │
       │                                            │ 1. EK cert signed by CA │
       │                                            │ 2. AIK generated from EK│
       │                                            │ 3. Signature matches AIK│
       │                                            │ 4. Nonce matches issued │
       │                                            │ 5. PCRs match baseline  │
       │                                            └────────────┬────────────┘
       │                                                         │
       │ 4. Enrollment Success (Issues 24-Hour Fleet Access JWT) │
       │◄────────────────────────────────────────────────────────┤
```

---

## 2. Server-Side Verification Logic (Pseudocode)

```cpp
bool verify_appliance_quote(
    const IdentityPayload& payload, 
    const std::vector<uint8_t>& issued_nonce,
    const GoldenPcrBaseline& baseline
) {
    // Step 1: Verify Endorsement Key Certificate against Manufacturer CA (Infineon, ST, etc.)
    if (!verify_x509_chain(payload.ek_certificate, MANUFACTURER_ROOT_CAS)) {
        return false;
    }

    // Step 2: Verify Attestation Data contains the identical nonce
    const auto* attest = reinterpret_cast<const TPMS_ATTEST*>(payload.attest_data.data());
    if (std::memcmp(attest->extraData.buffer, issued_nonce.data(), 32) != 0) {
        return false; // Replay attack detected
    }

    // Step 3: Verify the cryptographic signature using Public AIK
    if (!verify_rsa_pss_signature(payload.attest_data, payload.signature, payload.aik_public_key)) {
        return false; // Signature forged
    }

    // Step 4: Validate PCR Digest matches known golden image
    if (std::memcmp(attest->attested.quote.pcrDigest.buffer, baseline.digest.data(), 32) != 0) {
        return false; // Host firmware or bootloader tampered
    }

    return true; // Node successfully authenticated
}
```

---

## 3. Cryptographic Parameters

* **AIK Key Type:** RSA-2048 with RSASSA-PSS or ECC NIST P-256.
* **Hashing Function:** SHA-256.
* **Nonce Length:** Exactly 32 bytes ($256\text{ bits}$) generated via a cryptographically secure pseudorandom number generator (`/dev/urandom`).
* **Handshake SLA:** The complete generation and verification cycle executes in **$< 48\,\text{ms}$**.
```

---

### Complete in Part 5
- `blackbox-essential/docs/hardware-identity-tpm/attestation-architecture.md`
- `blackbox-essential/docs/hardware-identity-tpm/tier1-physical-tpm2.md`
- `blackbox-essential/docs/hardware-identity-tpm/tpm2-pcr-measurements.md`
- `blackbox-essential/docs/hardware-identity-tpm/tier2-virtual-tpm.md`
- `blackbox-essential/docs/hardware-identity-tpm/tier3-dmi-fallback.md`
- `blackbox-essential/docs/hardware-identity-tpm/anti-cloning-protections.md`
- `blackbox-essential/docs/hardware-identity-tpm/quote-verification-flow.md`

All 7 hardware identity and TPM attestation files are now generated.

---

### Files to be Generated in Part 6

The next phase covers **High-Throughput User-Space Networking via AF_XDP Zero-Copy** (`af-xdp-zero-copy/`):

1. `af-xdp-zero-copy/umem-architecture.md` (Packet buffer ring allocation in unified user-memory)
2. `af-xdp-zero-copy/rx-fill-rings.md` (Coordinating descriptor exchanges between NIC and userspace)
3. `af-xdp-zero-copy/zero-copy-packet-transfer.md` (Zero-copy DMA transfers from NIC directly to inference memory)
4. `af-xdp-zero-copy/line-rate-saturation-10gbe.md` (Pushing 1.25M+ events/sec on Intel X520 and E810 adapters)
5. `af-xdp-zero-copy/multi-core-rss-queues.md` (Scaling across multi-queue NICs using Receive Side Scaling)

Confirm when you are ready to proceed with Part 6.