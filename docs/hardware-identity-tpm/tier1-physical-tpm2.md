---

### File: `blackbox-essential/docs/hardware-identity-tpm/tier1-physical-tpm2.md`

```markdown
# Tier 1 Identity: Physical TPM 2.0 Interface (`/dev/tpmrm0`)

Tier 1 attestation interfaces directly with physical discrete or firmware TPM 2.0 silicon (such as Infineon OPTIGA, STMicroelectronics ST33, or Intel PTT) via the Linux TPM Resource Manager device (`/dev/tpmrm0`) and the TCG TSS2 Enhanced System API (`libtss2-esys`).

---

## 1. Interfacing via the TPM Resource Manager

Modern Linux kernels provide two character devices:
* `/dev/tpm0`: Direct hardware access (Exclusive access; causes bus contention).
* `/dev/tpmrm0`: **In-kernel Resource Manager** (Multi-process multiplexed; handles context swapping between user space and TPM SRAM).

`blackbox-essential` communicates exclusively through `/dev/tpmrm0`:

```bash
# Verify permissions and access to the resource manager
ls -l /dev/tpmrm0
# Output: crw-rw---- 1 tss tss 10, 224 /dev/tpmrm0
```

---

## 2. TSS2 C++20 Identity Driver Implementation

```cpp
#include <tss2/tss2_esys.h>
#include <blackbox/hardware_identity.hpp>
#include <stdexcept>
#include <vector>

namespace blackbox {

class Tpm2Driver {
public:
    Tpm2Driver() {
        // Initialize the ESYS context communicating with /dev/tpmrm0
        TSS2_RC rc = Esys_Initialize(&esys_context_, nullptr, nullptr);
        if (rc != TSS2_RC_SUCCESS) {
            throw std::runtime_error("Failed to initialize TSS2 ESYS context");
        }
    }

    ~Tpm2Driver() {
        if (esys_context_) {
            Esys_Finalize(&esys_context_);
        }
    }

    // Verify presence and read manufacturer capabilities
    std::string get_manufacturer_id() {
        TPMS_CAPABILITY_DATA* cap_data = nullptr;
        TSS2_RC rc = Esys_GetCapability(
            esys_context_,
            ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE,
            TPM2_CAP_TPM_PROPERTIES,
            TPM2_PT_MANUFACTURER,
            1,
            nullptr,
            &cap_data
        );

        if (rc != TSS2_RC_SUCCESS || !cap_data) {
            return "UNKNOWN";
        }

        uint32_t val = cap_data->data.tpmProperties.tpmProperty[0].value;
        char mfg[5] = {
            static_cast<char>((val >> 24) & 0xFF),
            static_cast<char>((val >> 16) & 0xFF),
            static_cast<char>((val >> 8) & 0xFF),
            static_cast<char>(val & 0xFF),
            '\0'
        };

        Esys_Free(cap_data);
        return std::string(mfg);
    }

private:
    ESYS_CONTEXT* esys_context_{nullptr};
};

} // namespace blackbox
```

---

## 3. Endorsement Primary Key Generation

The primary Endorsement Key (EK) is derived from the hardware root secret embedded within the TPM during chip manufacturing:

```cpp
ESYS_TR create_endorsement_key(ESYS_CONTEXT* ctx) {
    TPM2B_PUBLIC in_public{};
    in_public.publicArea.type = TPM2_ALG_RSA;
    in_public.publicArea.nameAlg = TPM2_ALG_SHA256;
    in_public.publicArea.objectAttributes = (
        TPMA_OBJECT_RESTRICTED |
        TPMA_OBJECT_DECRYPT |
        TPMA_OBJECT_FIXEDTPM |
        TPMA_OBJECT_FIXEDPARENT |
        TPMA_OBJECT_ADMINWITHPOLICY
    );
    in_public.publicArea.parameters.rsaDetail.symmetric.algorithm = TPM2_ALG_AES;
    in_public.publicArea.parameters.rsaDetail.symmetric.keyBits.aes = 128;
    in_public.publicArea.parameters.rsaDetail.symmetric.mode.aes = TPM2_ALG_CFB;
    in_public.publicArea.parameters.rsaDetail.scheme.scheme = TPM2_ALG_NULL;
    in_public.publicArea.parameters.rsaDetail.keyBits = 2048;

    ESYS_TR ek_handle = ESYS_TR_NONE;
    TSS2_RC rc = Esys_CreatePrimary(
        ctx,
        ESYS_TR_RH_ENDORSEMENT,
        ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE,
        nullptr, &in_public, nullptr, nullptr,
        &ek_handle, nullptr, nullptr, nullptr, nullptr
    );

    if (rc != TSS2_RC_SUCCESS) {
        throw std::runtime_error("Failed to create TPM 2.0 Primary EK");
    }

    return ek_handle;
}
```
```

---

### File: `blackbox-essential/docs/hardware-identity-tpm/tpm2-pcr-measurements.md`

```markdown
# Generating Cryptographic Quotes over PCR 0 and PCR 4

A **TPM2 Quote** is a cryptographically signed statement produced by the TPM that certifies the current contents of its Platform Configuration Registers (PCRs) alongside an external freshness nonce.

---

## 1. Target PCR Selection

`blackbox-essential` binds its identity claims to two foundational hardware registers:

| PCR Index | Measured System Component | Threat Mitigated |
| :--- | :--- | :--- |
| **PCR 0** | Core UEFI Firmware, BIOS Code, CPU Microcode | Rootkits, firmware modification, SPI flash tampering. |
| **PCR 4** | Bootloader (GRUB), Kernel Command Line, Initial Ramdisk | Malicious kernel boot parameters, rogue initrd injectors. |
| **PCR 7** | Secure Boot Certificates and Platform Keys (PK, KEK, db) | Unauthorized bootloader substitutions. |

---

## 2. Generating the Quote via `Esys_Quote()`

```cpp
#include <tss2/tss2_esys.h>
#include <blackbox/hardware_identity.hpp>
#include <span>

namespace blackbox {

struct PcrQuote {
    std::vector<uint8_t> signature;
    std::vector<uint8_t> pcr_digest;
    std::vector<uint8_t> attest_data;
    uint32_t pcr_mask;
};

PcrQuote generate_tpm2_quote(
    ESYS_CONTEXT* ctx, 
    ESYS_TR aik_handle, 
    std::span<const uint8_t, 32> nonce
) {
    // 1. Select PCR 0 and PCR 4
    TPML_PCR_SELECTION pcr_selection{};
    pcr_selection.count = 1;
    pcr_selection.pcrSelections[0].hash = TPM2_ALG_SHA256;
    pcr_selection.pcrSelections[0].sizeofSelect = 3;
    // Set bits 0 and 4: 0b00010001 = 0x11
    pcr_selection.pcrSelections[0].pcrSelect[0] = 0x11;

    // 2. Set nonce (Qualifying Data) to prevent replay attacks
    TPM2B_DATA qualifying_data{};
    qualifying_data.size = nonce.size();
    std::memcpy(qualifying_data.buffer, nonce.data(), nonce.size());

    // 3. Request Quote from TPM Hardware
    TPMT_SIG_SCHEME sig_scheme{};
    sig_scheme.scheme = TPM2_ALG_RSASSA;
    sig_scheme.details.rsassa.hashAlg = TPM2_ALG_SHA256;

    TPM2B_ATTEST* attest = nullptr;
    TPMT_SIGNATURE* signature = nullptr;

    TSS2_RC rc = Esys_Quote(
        ctx,
        aik_handle,
        ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE,
        &qualifying_data,
        &sig_scheme,
        &pcr_selection,
        &attest,
        &signature
    );

    if (rc != TSS2_RC_SUCCESS) {
        throw std::runtime_error("TPM2_Quote execution failed");
    }

    // 4. Serialize Quote Data
    PcrQuote quote;
    quote.signature.assign(
        signature->signature.rsassa.sig.buffer,
        signature->signature.rsassa.sig.buffer + signature->signature.rsassa.sig.size
    );
    quote.attest_data.assign(attest->attestationData, attest->attestationData + attest->size);
    quote.pcr_mask = 0x11;

    Esys_Free(attest);
    Esys_Free(signature);

    return quote;
}

} // namespace blackbox
```

---

## 3. Verifying Quote Freshness

The fleet command server validates the quote:
1. Re-computes the SHA-256 hash of the baseline golden PCR 0 and PCR 4 values.
2. Checks that `attest->extraData` exactly matches the originally issued nonce.
3. Validates the RSA/ECC digital signature using the device's public AIK certificate.
```

---

### File: `blackbox-essential/docs/hardware-identity-tpm/tier2-virtual-tpm.md`

```markdown
# Tier 2 Identity: Virtual TPM (vTPM / swtpm)

When deployed in virtualized enterprise datacenters or cloud environments (such as **VMware vSphere/ESXi, KVM/QEMU, or Microsoft Hyper-V**), physical discrete TPMs are typically inaccessible. `blackbox-essential` detects and interfaces with the hypervisor-provided **Virtual TPM (vTPM)**.

---

## 1. Distinguishing Physical TPM from vTPM

`blackbox::HardwareIdentity` inspects device vendor properties and ACPI tables to classify whether the underlying cryptoprocessor is physical silicon or an emulated hypervisor worker:

```cpp
#include <blackbox/hardware_identity.hpp>
#include <fstream>
#include <string>

namespace blackbox {

IdentityTier detect_tpm_tier(const std::string& manufacturer) {
    // Hypervisor vTPM vendor identifiers:
    // "VMW " -> VMware vSphere vTPM
    // "MSFT" -> Microsoft Hyper-V Virtual TPM
    // "SW  " -> QEMU software TPM (swtpm)
    if (manufacturer == "VMW " || manufacturer == "MSFT" || manufacturer == "SW  ") {
        return IdentityTier::TIER2_VTPM;
    }
    
    // Discrete / Physical Silicon Vendors:
    // "IFX " -> Infineon
    // "STM " -> STMicroelectronics
    // "NTC " -> Nuvoton
    // "INTC" -> Intel Firmware TPM (PTT)
    // "AMD " -> AMD Platform Security Processor (fTPM)
    return IdentityTier::TIER1_PHYSICAL_TPM;
}

} // namespace blackbox
```

---

## 2. VMware vSphere vTPM Specifics

In VMware vSphere environments, the vTPM state is stored within the virtual machine's `.nvram` file, encrypted via the ESXi Key Management Server (KMS).

* **Certificate Authority:** vTPM Endorsement Key certificates are signed by the virtualization platform's internal CA rather than a physical semiconductor manufacturer (e.g., Infineon).
* **PCR Validity:** PCR 0 measures the virtual BIOS (VMware EFI), while PCR 4 measures the virtualized bootloader.
* **Hypervisor Attestation Binding:** `blackbox-essential` pairs the vTPM quote with VMware Guest RPC queries (`vmware-rpctool`) to verify the virtual machine UUID assigned by vCenter.
```

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