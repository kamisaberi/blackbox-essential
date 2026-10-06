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
