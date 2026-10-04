# ModelConfig Schema

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


JSON/YAML declarative configuration schema definition.

## Sections

model_name, input_tensor, output_tensors, mitigation_thresholds.

## Validation

Schema-checked at load; unknown keys warn, type errors abort.

```json
{
  "model_name": "network_threat_v2.onnx",
  "input_tensor": {"name": "input_features", "dimensions": [1, 32]},
  "mitigation_thresholds": {"in_kernel_drop_threshold": 0.85}
}
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
