# Dynamic Tensor Binding: Decoupling C++ Engines from AI Models

Hardcoding feature extraction logic into low-level C++ network engines creates tight coupling: every time a data science team updates a model's feature set (e.g., adding TLS JA4 fingerprints or expanding from a 32-dim to an 80-dim flow vector), the kernel-adjacent C++ binary must be recompiled, tested, and redeployed.

`blackbox-essential` resolves this via **Dynamic Tensor Binding**, allowing the engine to ingest, normalize, and pack network telemetry into arbitrary tensor shapes defined declaratively at runtime.

---

## 1. The Decoupled Abstraction

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ Raw Network Ingress: AF_XDP / eBPF Flow Telemetry           │
 │ (IP Headers, TCP Flags, Inter-Arrival Times, Window Sizes)  │
 └──────────────────────────────┬──────────────────────────────┘
                                │
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ blackbox::ModelConfig Subsystem (Dynamic Feature Arbiter)   │
 │   - Reads model_config.yaml at startup                      │
 │   - Maps internal feature IDs to arbitrary vector positions │
 │   - Applies scaling, clamping, and log1p transformations    │
 └──────────────────────────────┬──────────────────────────────┘
                                │
         ┌──────────────────────┼──────────────────────┐
         ▼                      ▼                      ▼
 ┌───────────────┐      ┌───────────────┐      ┌───────────────┐
 │ Model A (v1)  │      │ Model B (v2)  │      │ Model C (ICS) │
 │ 32-Dim NetFlow│      │ 42-Dim Hybrid │      │ 80-Dim CIC-IDS│
 │ Tabular Vector│      │ SCADA Flow    │      │ Full Topology │
 └───────────────┘      └───────────────┘      └───────────────┘
```

---

## 2. Dynamic Feature Extractor Interface

Instead of populating an array at fixed memory offsets, the engine references an extensible feature registry:

```cpp
#include <blackbox/model_config.hpp>
#include <xinfer/tensor.hpp>
#include <span>

namespace blackbox {

class DynamicTensorBinder {
public:
    explicit DynamicTensorBinder(const ModelConfig& config);

    // Maps raw flow records directly into an inference tensor buffer
    void bind_flow_to_tensor(
        const RawFlowTelemetry& flow, 
        std::shared_ptr<xinfer::Tensor>& tensor
    ) const {
        float* dst = tensor->data<float>();
        
        for (size_t i = 0; i < feature_bindings_.size(); ++i) {
            const auto& binding = feature_bindings_[i];
            
            // Extract raw feature value based on configured enum ID
            double raw_value = extract_raw_metric(flow, binding.feature_id);

            // Apply configured mathematical normalization
            dst[i] = static_cast<float>(binding.normalize(raw_value));
        }
    }

private:
    std::vector<FeatureBindingRule> feature_bindings_;
    static double extract_raw_metric(const RawFlowTelemetry& flow, FeatureId id);
};

} // namespace blackbox
```

---

## 3. Operational Invariants

1. **Zero Runtime Allocation:** Once initialized, the dynamic binder executes over pre-allocated contiguous arrays, performing zero heap allocations during live packet inspection.
2. **Schema Validation on Load:** Tensor dimensions declared in the configuration are verified against the loaded ONNX/RKNN model's input shape before the engine attaches to network interfaces.

