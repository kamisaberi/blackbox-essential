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

