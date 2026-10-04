---

### File: `blackbox-essential/docs/model-config/mapping-tensor-dimensions.md`

```markdown
# Mapping Arbitrary Tensor Dimensions (32-dim, 42-dim, 80-dim)

`blackbox-essential` supports diverse neural network topologies by providing pre-built mapping dictionaries for common security benchmarks and industrial telemetry profiles.

---

## 1. Supported Dimension Profiles

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ 32-Dimensional Default NetFlow Vector (Default Autoencoder) │
 │  - Features: Flow duration, bytes/packets in/out, flag      │
 │    counts (SYN, RST, PSH, ACK), min/max/mean packet lengths,│
 │    TCP window statistics, and protocol encodings.           │
 └─────────────────────────────────────────────────────────────┘
                               ▲
                               │
 ┌─────────────────────────────┴───────────────────────────────┐
 │ 42-Dimensional Extended Cyber-Physical SCADA Profile        │
 │  - Features: The standard 32 NetFlow features PLUS 10 Modbus│
 │    and DNP3 application layer metrics (Function Code, Coil  │
 │    Quantity, Register Write Spikes, APDU Entropy).          │
 └─────────────────────────────┬───────────────────────────────┘
                               │
                               ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ 80-Dimensional Academic Benchmark Profile (CIC-IDS-2017)     │
 │  - Features: Comprehensive statistical telemetry including  │
 │    bidirectional inter-arrival time standard deviations,    │
 │    sub-flow forward/backward metrics, and active/idle means.│
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. Normalization Mathematics

To keep model weights stable, feature values are normalized into bounded ranges using three mathematical strategies:

### 1. Logarithmic Scaling (`LOG1P`)
Used for wide-range fields such as flow duration, total byte counts, and packet counts:
$$f(x) = \ln(1.0 + x)$$

### 2. Linear Min-Max Scaling (`MINMAX`)
Used for known protocol bounds (e.g., packet lengths $\in [0, 1500]$, TCP window sizes $\in [0, 65535]$):
$$f(x) = \text{clip}\left(\frac{x - x_{\text{min}}}{x_{\text{max}} - x_{\text{min}}},\, 0.0,\, 1.0\right)$$

### 3. Standard Score Normalization (`ZSCORE`)
Used for Gaussian distributed metrics like inter-arrival times:
$$f(x) = \frac{x - \mu}{\sigma}$$

---

## 3. C++ Normalization Implementation

```cpp
namespace blackbox {

enum class TransformType : uint8_t {
    NONE = 0,
    LOG1P,
    MINMAX,
    ZSCORE
};

struct FeatureBindingRule {
    FeatureId feature_id;
    TransformType transform;
    double min_val{0.0};
    double max_val{1.0};
    double mean{0.0};
    double stddev{1.0};

    [[nodiscard]] inline double normalize(double x) const noexcept {
        switch (transform) {
            case TransformType::LOG1P:
                return std::log1p(x > 0.0 ? x : 0.0);
            case TransformType::MINMAX: {
                double clamped = std::clamp(x, min_val, max_val);
                return (clamped - min_val) / (max_val - min_val);
            }
            case TransformType::ZSCORE:
                return (x - mean) / (stddev > 0.00001 ? stddev : 1.0);
            default:
                return x;
        }
    }
};

} // namespace blackbox
```
```

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