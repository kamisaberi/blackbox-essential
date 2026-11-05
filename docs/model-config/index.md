# Model Config & Dynamic Tensor Binding

The `blackbox::ModelConfig` subsystem decouples C++ packet engines from machine learning models through declarative YAML specifications.

## Section Navigation

- [Dynamic Tensor Binding](./dynamic-tensor-binding.md): Extensible feature registry mapping raw flow metrics to tensor positions.
- [Mapping Tensor Dimensions](./mapping-tensor-dimensions.md): Pre-built feature maps for 32-dim, 42-dim, and 80-dim vectors, plus normalization math.
- [Model Config Schema](./model-config-schema.md): Formal YAML specification (`model_config.yaml`) and validation rules.
- [Threshold & Mitigation Rules](./threshold-and-mitigation-rules.md): Translating output probabilities into in-kernel drop rules and dynamic TTL escalation.
