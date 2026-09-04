#pragma once

#include "blackbox/export.hpp"
#include <string>

namespace blackbox {

enum class TPMStatus {
    PhysicalHardwareTPM, // Discrete physical TPM 2.0 chip (Infineon, Nuvoton, STMicro)
    VirtualTPM,          // VMware vTPM, QEMU swtpm, or Cloud vTPM
    SoftwareFallback     // No TPM present; using Motherboard DMI UUID + Machine-ID
};

struct BLACKBOX_API HardwareNodeInfo {
    TPMStatus tpm_status;
    std::string tpm_description;
    std::string hardware_uuid;
    std::string unique_node_fingerprint;
};

class BLACKBOX_API HardwareIdentity {
public:
    // Automatically inspects environment and returns hardware identity
    static HardwareNodeInfo get_node_info();

    // Helper conversion
    static std::string status_to_string(TPMStatus status);

private:
    static std::string read_sysfs_file(const std::string& path);
    static std::string compute_sha256(const std::string& input);
};

} // namespace blackbox