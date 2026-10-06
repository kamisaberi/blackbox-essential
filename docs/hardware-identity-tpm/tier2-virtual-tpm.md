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

