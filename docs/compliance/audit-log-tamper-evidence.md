# Cryptographic Tamper-Evident Audit Logging

Regulatory compliance frameworks (including NIS 2, CMMC 2.0, and IEC 62443) mandate that security audit trails and mitigation records remain **immutable and tamper-evident**. 

`blackbox-essential` secures its event logs using **Forward-Secure Cryptographic Hash Chaining** anchored to non-volatile physical TPM registers.

---

## 1. Cryptographic Hash Chain Architecture

Mitigation events emitted by the lock-free SPMC ring buffer are sequenced into an unbroken cryptographic hash chain:

$$H_0 = \text{TPM\_BOOT\_SEED}$$

$$H_i = \text{SHA-256}\left(H_{i-1} \parallel \text{Timestamp}_i \parallel \text{SourceIP}_i \parallel \text{Action}_i \parallel \text{DropCount}_i\right)$$

```text
 Event 1 (Drop IP A)        Event 2 (Drop IP B)        Event 3 (Drop IP C)
 ┌──────────────────────┐   ┌──────────────────────┐   ┌──────────────────────┐
 │ Hash H_1             │   │ Hash H_2             │   │ Hash H_3             │
 │ SHA256(H_0 || Evt_1) │──►│ SHA256(H_1 || Evt_2) │──►│ SHA256(H_2 || Evt_3) │
 └──────────────────────┘   └──────────────────────┘   └──────────┬───────────┘
                                                                  │
                                                                  ▼ Every 300 Seconds
                                                       ┌──────────────────────┐
                                                       │ TPM 2.0 PCR Extend   │
                                                       │ TPM2_PCR_Extend(     │
                                                       │   PCR_12, H_3)       │
                                                       └──────────────────────┘
```

If an adversary with root privileges modifies, deletes, or reorders a historical log entry:
1. The recalculated hash chain diverges immediately: $H_i' \ne H_i$.
2. The periodic hardware hash extending into **TPM PCR 12** will not match the calculated root, providing mathematical proof of log tampering to auditors.

---

## 2. C++20 Hash-Chaining Logger Implementation

```cpp
#include <openssl/sha.h>
#include <blackbox/telemetry.hpp>
#include <vector>
#include <iomanip>
#include <sstream>

namespace blackbox {

class TamperEvidentLog {
public:
    explicit TamperEvidentLog(const std::string& boot_seed) {
        SHA256(reinterpret_cast<const uint8_t*>(boot_seed.data()), boot_seed.size(), current_hash_);
    }

    // Appends an event and updates the forward cryptographic state
    std::string append_event(const FlowEvent& event, MitigationAction action) {
        std::ostringstream ss;
        ss << std::string(reinterpret_cast<char*>(current_hash_), SHA256_DIGEST_LENGTH)
           << event.timestamp_ns
           << event.src_ip
           << static_cast<int>(action);

        std::string payload = ss.str();
        SHA256(reinterpret_cast<const uint8_t*>(payload.data()), payload.size(), current_hash_);

        return get_current_hash_hex();
    }

    std::string get_current_hash_hex() const {
        std::ostringstream hex_stream;
        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            hex_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(current_hash_[i]);
        }
        return hex_stream.str();
    }

private:
    uint8_t current_hash_[SHA256_DIGEST_LENGTH]{};
};

} // namespace blackbox
```

---

## 3. Auditor Verification Command

To verify log chain integrity:

```bash
blackbox-ctl audit verify --log-file /var/log/blackbox/mitigation.log --pcr-index 12
```

### Expected Output
```text
[*] Validating 142,891 cryptographic audit log records...
[+] Hash Chain Integrity: VERIFIED (0 Anomalies, 0 Deleted Frames)
[+] PCR 12 Hardware Seal : MATCHED (Hardware state authenticates log integrity)
[+] Result: AUDIT LOG IS TAMPER-EVIDENT AND COMPLIANT.
```

