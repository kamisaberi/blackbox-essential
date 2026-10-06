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

