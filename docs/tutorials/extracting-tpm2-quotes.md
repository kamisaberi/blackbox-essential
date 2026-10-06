# Reading and Verifying Physical TPM 2.0 Silicon Quotes

This tutorial guides you through extracting a non-spoofable hardware attestation quote from a physical TPM 2.0 chip using `blackbox::HardwareIdentity`, measuring PCR 0 and PCR 4, and verifying the quote against an external cryptographic nonce.

---

## 1. Attestation Sequence

```text
 1. Generate 32-byte cryptographic random nonce (Anti-replay token)
                            │
                            ▼
 2. Initialize blackbox::HardwareIdentity::instance()
                            │
                            ▼
 3. Invoke generate_claims(nonce)
    ├── Reads Platform Configuration Registers (PCR 0, PCR 4)
    ├── Derives Attestation Identity Key (AIK) from Endorsement Key
    └── Hardware signs (PCR_Digest + Nonce) via RSASSA-PSS
                            │
                            ▼
 4. Verify IdentityClaims against nonce and golden firmware baselines
```

---

## 2. Complete C++20 Implementation (`tpm_attestation.cpp`)

```cpp
#include <blackbox/hardware_identity.hpp>
#include <random>
#include <iostream>
#include <iomanip>

int main() {
    std::cout << "====================================================\n"
              << "     Blackbox-Essential TPM 2.0 Attestation Harness \n"
              << "====================================================\n";

    auto& identity = blackbox::HardwareIdentity::instance();

    // 1. Inspect Hardware Root of Trust
    std::cout << "[+] Active Identity Tier : " 
              << (identity.active_tier() == blackbox::IdentityTier::TIER1_PHYSICAL_TPM ? "TIER 1 (Physical TPM 2.0)" :
                  identity.active_tier() == blackbox::IdentityTier::TIER2_VTPM ? "TIER 2 (Virtual TPM)" : "TIER 3 (DMI Fallback)")
              << "\n"
              << "[+] Silicon Manufacturer : " << identity.manufacturer_id() << "\n"
              << "[+] Unique Platform UUID : " << identity.get_unique_identifier() << "\n\n";

    // 2. Generate 32-Byte Cryptographic Nonce (Simulating challenge from Sentinel-Nexus)
    std::array<uint8_t, 32> challenge_nonce{};
    std::random_device rd;
    for (size_t i = 0; i < 32; ++i) {
        challenge_nonce[i] = static_cast<uint8_t>(rd());
    }

    std::cout << "[*] Generated Challenge Nonce: ";
    for (auto byte : challenge_nonce) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    std::cout << std::dec << "\n[*] Requesting TPM 2.0 hardware signature over PCR 0 and PCR 4...\n";

    // 3. Generate Hardware Quote
    blackbox::IdentityClaims claims;
    try {
        claims = identity.generate_claims(challenge_nonce);
        std::cout << "[+] TPM 2.0 Quote generated successfully in hardware silicon!\n";
    } catch (const blackbox::BlackboxException& ex) {
        std::cerr << "[-] TPM Attestation Failed: " << ex.what() << "\n";
        return 1;
    }

    // 4. Output Cryptographic Artifacts
    std::cout << "\n---------------- ATTESTATION ARTIFACTS ----------------\n"
              << "PCR Mask               : 0x" << std::hex << claims.pcr_mask << std::dec << " (PCR 0 & PCR 4)\n"
              << "Quote Signature Size   : " << claims.quote_signature.size() << " bytes\n"
              << "Public AIK Cert Size   : " << claims.aik_public_cert.size() << " bytes\n"
              << "Timestamp (Monotonic)  : " << claims.timestamp_ns << " ns\n";

    // 5. Verify the Quote locally
    bool valid = identity.verify_claims(claims, challenge_nonce);
    std::cout << "Local Signature Verification: " << (valid ? "PASSED (AUTHENTIC SILICON)" : "FAILED") << "\n";

    return valid ? 0 : 1;
}
```

---

## 3. Compilation & Execution

Link against the TPM Software Stack libraries:

```bash
clang++-16 -std=c++20 tpm_attestation.cpp -o tpm_attestation \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lblackbox -ltss2-esys -ltss2-rc -lcrypto \
    -Wl,-rpath,/usr/local/lib

sudo ./tpm_attestation
```

### Expected Output

```text
====================================================
     Blackbox-Essential TPM 2.0 Attestation Harness 
====================================================
[+] Active Identity Tier : TIER 1 (Physical TPM 2.0)
[+] Silicon Manufacturer : IFX (Infineon)
[+] Unique Platform UUID : 8a2f3c1e-847b-42c9-94b1-3a7b41e2d901

[*] Generated Challenge Nonce: 7f4a8b...3c12
[*] Requesting TPM 2.0 hardware signature over PCR 0 and PCR 4...
[+] TPM 2.0 Quote generated successfully in hardware silicon!

---------------- ATTESTATION ARTIFACTS ----------------
PCR Mask               : 0x11 (PCR 0 & PCR 4)
Quote Signature Size   : 256 bytes (RSA-2048 PSS)
Public AIK Cert Size   : 412 bytes
Timestamp (Monotonic)  : 1842918471209 ns
Local Signature Verification: PASSED (AUTHENTIC SILICON)
```

