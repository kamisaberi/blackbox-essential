# Class `blackbox::HardwareIdentity`

Defined in header `<blackbox/hardware_identity.hpp>`  
Namespace: `blackbox`

`HardwareIdentity` provides cryptographic device attestation, managing interactions with physical TPM 2.0 chips, hypervisor vTPMs, and DMI fallback identifiers.

---

## 1. Class Synopsis

```cpp
namespace blackbox {

class BLACKBOX_API HardwareIdentity {
public:
    static HardwareIdentity& instance() noexcept;

    // Identity Discovery
    [[nodiscard]] IdentityTier active_tier() const noexcept;
    [[nodiscard]] std::string get_unique_identifier();
    [[nodiscard]] std::string manufacturer_id() const;
    [[nodiscard]] bool is_hardware_rooted() const noexcept;

    // Cryptographic Quote Generation
    [[nodiscard]] IdentityClaims generate_claims(std::span<const uint8_t, 32> nonce);
    [[nodiscard]] bool verify_claims(
        const IdentityClaims& claims, 
        std::span<const uint8_t, 32> nonce
    ) const;

    // Emergency Revocation
    void handle_revocation() noexcept;

private:
    HardwareIdentity();
    ~HardwareIdentity();
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace blackbox
```

---

## 2. Key Member Functions

### `generate_claims`
```cpp
IdentityClaims generate_claims(std::span<const uint8_t, 32> nonce);
```
Generates a signed TPM 2.0 quote certifying PCR 0 (BIOS) and PCR 4 (Bootloader) bound to the provided single-use 32-byte `nonce`. Returns an `IdentityClaims` structure containing the digital signature, PCR digest, and public certificates.

---

### `active_tier`
```cpp
IdentityTier active_tier() const noexcept;
```
Returns the operational identity level: `TIER1_PHYSICAL_TPM`, `TIER2_VTPM`, or `TIER3_DMI_FALLBACK`.

