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

