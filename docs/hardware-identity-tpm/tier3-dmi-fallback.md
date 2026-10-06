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

