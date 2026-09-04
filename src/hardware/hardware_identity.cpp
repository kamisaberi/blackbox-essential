#include "blackbox/hardware_identity.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/sha.h>

namespace blackbox {

std::string HardwareIdentity::read_sysfs_file(const std::string& path) {
    std::ifstream file(path);
    if (file.is_open()) {
        std::string content;
        file >> content;
        return content;
    }
    return "";
}

std::string HardwareIdentity::compute_sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);

    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::string HardwareIdentity::status_to_string(TPMStatus status) {
    switch (status) {
        case TPMStatus::PhysicalHardwareTPM: return "Physical Hardware TPM 2.0";
        case TPMStatus::VirtualTPM:          return "Virtual TPM 2.0 (vTPM / Hypervisor)";
        case TPMStatus::SoftwareFallback:     return "Software Fallback (DMI UUID + Machine-ID)";
        default:                             return "Unknown";
    }
}

HardwareNodeInfo HardwareIdentity::get_node_info() {
    HardwareNodeInfo info;

    // 1. Check for TPM character devices in kernel
    int tpm_fd = open("/dev/tpmrm0", O_RDWR);
    if (tpm_fd < 0) {
        tpm_fd = open("/dev/tpm0", O_RDWR);
    }

    if (tpm_fd >= 0) {
        close(tpm_fd);

        // Check description in sysfs to determine if it's physical or virtual
        std::string desc = read_sysfs_file("/sys/class/tpm/tpm0/device/description");
        if (desc.empty()) {
            desc = read_sysfs_file("/sys/class/tpm/tpm0/device/id");
        }

        info.tpm_description = desc.empty() ? "TPM 2.0 Device" : desc;

        // Detect Hypervisor vTPM signatures (VMware, QEMU, Xen, Microsoft)
        if (desc.find("VMW") != std::string::npos || 
            desc.find("QEMU") != std::string::npos || 
            desc.find("MSFT") != std::string::npos ||
            desc.find("swtpm") != std::string::npos) {
            info.tpm_status = TPMStatus::VirtualTPM;
        } else {
            info.tpm_status = TPMStatus::PhysicalHardwareTPM;
        }

    } else {
        // No TPM device file present -> Graceful Software Fallback
        info.tpm_status = TPMStatus::SoftwareFallback;
        info.tpm_description = "None (Simulated Node Identity)";
    }

    // 2. Read Motherboard DMI UUID
    std::string dmi_uuid = read_sysfs_file("/sys/class/dmi/id/product_uuid");
    if (dmi_uuid.empty()) {
        dmi_uuid = read_sysfs_file("/etc/machine-id");
    }
    if (dmi_uuid.empty()) {
        dmi_uuid = "BLACKBOX-GENERIC-NODE-001";
    }
    info.hardware_uuid = dmi_uuid;

    // 3. Generate Cryptographic Fingerprint for Licensing
    std::string raw_identity = info.hardware_uuid + ":" + info.tpm_description;
    info.unique_node_fingerprint = compute_sha256(raw_identity).substr(0, 32);

    return info;
}

} // namespace blackbox