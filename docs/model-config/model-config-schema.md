---

### File: `blackbox-essential/docs/model-config/model-config-schema.md`

```markdown
# Declarative Model Configuration Schema (`model_config.yaml`)

The `blackbox::ModelConfig` subsystem is configured through an immutable YAML manifest. This file declares model metadata, input/output tensor shapes, normalization rules, and mitigation thresholds.

---

## 1. Formal YAML Schema Specification

```yaml
version: "1.0.0"

metadata:
  model_name: "network_threat_autoencoder"
  model_version: "2.4.0"
  model_format: "ONNX"
  sha256_hash: "e9a2c31e847b2c94b13a7b41e200000000000000000000000000000000000000"
  target_silicon: "AUTO"

tensor_input:
  name: "flow_features"
  dimensions: [1, 32]
  precision: "FP32"
  features:
    - id: "DURATION"
      transform: "LOG1P"
    - id: "PACKETS_SRC_TO_DST"
      transform: "LOG1P"
    - id: "PACKETS_DST_TO_SRC"
      transform: "LOG1P"
    - id: "BYTES_SRC_TO_DST"
      transform: "LOG1P"
    - id: "BYTES_DST_TO_SRC"
      transform: "LOG1P"
    - id: "PACKET_LENGTH_MIN"
      transform: "MINMAX"
      min: 0.0
      max: 1500.0
    - id: "PACKET_LENGTH_MAX"
      transform: "MINMAX"
      min: 0.0
      max: 1500.0
    - id: "TCP_FLAGS_SYN"
      transform: "NONE"
    - id: "TCP_FLAGS_RST"
      transform: "NONE"
    - id: "TCP_WINDOW_SRC"
      transform: "MINMAX"
      min: 0.0
      max: 65535.0

tensor_output:
  name: "reconstruction_output"
  dimensions: [1, 32]
  evaluation_metric: "MSE_RECONSTRUCTION_LOSS"

mitigation:
  thresholds:
    anomaly_threshold: 0.082
    high_confidence_threshold: 0.250
  policies:
    on_anomaly: "XDP_DROP"
    on_clean: "XDP_PASS"
  ttl:
    default_ttl_seconds: 60
    escalation_ttl_seconds: 3600
```

---

## 2. Schema Validation Rules

When `libblackbox.so` parses `model_config.yaml`:
1. **Length Invariant:** The number of declared elements in the `features` array **must exactly match** the secondary dimension of `tensor_input.dimensions` (e.g., exactly 32 features for `[1, 32]`).
2. **Cryptographic Check:** The SHA-256 hash defined in `metadata.sha256_hash` is verified against the physical model binary on disk via `xinfer::ModelHub` before execution begins.
3. **Strict Bounds:** If `transform: "MINMAX"` is declared, `max` must be strictly greater than `min`.
```

---

### File: `blackbox-essential/docs/model-config/threshold-and-mitigation-rules.md`

```markdown
# Mapping Model Probabilities to In-Kernel Drop Actions

Once an inference pass completes, continuous output values (such as an autoencoder reconstruction loss or a classification probability) must be translated into discrete, microsecond packet mitigation decisions.

---

## 1. Multi-Tier Escalation Architecture

```text
 Ingress Packet -> Flow Ingestion -> Inference Execution
                                           │
                                           ▼ Evaluation Score (S)
 ┌─────────────────────────────────────────────────────────────────────────────┐
 │ Policy Evaluation Matrix                                                    │
 └─────────────────────────────────────────────────────────────────────────────┘
      │                                   │                               │
      │ S < 0.082                         │ 0.082 <= S < 0.250            │ S >= 0.250
      ▼ (Clean Flow)                      ▼ (Suspicious Deviation)        ▼ (Confirmed Threat)
 ┌─────────────────────────┐         ┌─────────────────────────┐     ┌─────────────────────────┐
 │ Action: XDP_PASS        │         │ Action: Rate Limit /    │     │ Action: XDP_DROP        │
 │ Permitted to Linux stack│         │ Telemetry Flagging      │     │ Insert into in-kernel   │
 │                         │         │ Staged for review       │     │ blocked_ip_map (TTL=60s)│
 └─────────────────────────┘         └─────────────────────────┘     └─────────────────────────┘
```

---

## 2. In-Kernel Action Trigger Implementation

```cpp
#include <blackbox/xdp_manager.hpp>
#include <blackbox/model_config.hpp>

namespace blackbox {

class MitigationArbiter {
public:
    MitigationArbiter(XdpManager& xdp, const MitigationPolicyConfig& policy)
        : xdp_(xdp), policy_(policy) {}

    void evaluate_and_enforce(uint32_t src_ipv4, double score) {
        if (score >= policy_.high_confidence_threshold) {
            // Tier 2 Escalation: Confirmed exploit
            // Block in kernel driver space for extended TTL (e.g., 1 hour)
            xdp_.block_ip(src_ipv4, policy_.escalation_ttl_seconds);
            log_security_event(src_ipv4, score, MitigationAction::KERNEL_DROP_ESCALATED);

        } else if (score >= policy_.anomaly_threshold) {
            // Tier 1 Active Defense: Ephemeral kernel drop
            // Block in kernel driver space for standard TTL (e.g., 60 seconds)
            xdp_.block_ip(src_ipv4, policy_.default_ttl_seconds);
            log_security_event(src_ipv4, score, MitigationAction::KERNEL_DROP);

        } else {
            // Benign flow: No kernel action required; packet passes via XDP_PASS
        }
    }

private:
    XdpManager& xdp_;
    MitigationPolicyConfig policy_;
    void log_security_event(uint32_t ip, double score, MitigationAction action);
};

} // namespace blackbox
```

---

## 3. Dynamic TTL Adaptation

Under active attack conditions, `MitigationArbiter` scales TTL durations dynamically:
* **First Offense:** Blocks for $60\,\text{seconds}$.
* **Repeated Violations:** If an IP triggers an anomaly again within a 10-minute window, the TTL scales exponentially:

$$\text{TTL}_{\text{new}} = \min\left(\text{TTL}_{\text{base}} \times 2^{\text{repeat\_count}},\, 86400\,\text{s}\right)$$

This mechanism shields critical infrastructure while allowing transient network glitches to clear automatically without manual operator intervention.
```

---

### Complete in Part 7
- `blackbox-essential/docs/model-config/dynamic-tensor-binding.md`
- `blackbox-essential/docs/model-config/mapping-tensor-dimensions.md`
- `blackbox-essential/docs/model-config/model-config-schema.md`
- `blackbox-essential/docs/model-config/threshold-and-mitigation-rules.md`

All 4 Model Configuration documentation files are now generated.

---

### Files to be Generated in Part 8

The next phase covers the complete **C++20 API Reference** (`api-reference/`):

1. `api-reference/index.md` (Namespace overview `blackbox::`)
2. `api-reference/xdp-manager.md` (Class `blackbox::XdpManager`)
3. `api-reference/event-ring-buffer.md` (Class `blackbox::EventRingBuffer`)
4. `api-reference/hardware-identity.md` (Class `blackbox::HardwareIdentity`)
5. `api-reference/model-config.md` (Class `blackbox::ModelConfig`)
6. `api-reference/kernel-telemetry.md` (Struct `blackbox::KernelTelemetry`)
7. `api-reference/data-structures.md` (Structs `XdpConfig`, `IdentityClaims`, `BlockedIpEntry`)
8. `api-reference/error-codes.md` (Class `blackbox::BlackboxException` & return codes)

Confirm when you are ready to proceed with Part 8.