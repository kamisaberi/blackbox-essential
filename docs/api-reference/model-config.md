# Class `blackbox::ModelConfig`

Defined in header `<blackbox/model_config.hpp>`  
Namespace: `blackbox`

`ModelConfig` parses declarative YAML model manifests and coordinates dynamic tensor bindings between incoming network frames and neural network input layers.

---

## 1. Class Synopsis

```cpp
namespace blackbox {

class BLACKBOX_API ModelConfig {
public:
    static ModelConfig load_from_file(const std::filesystem::path& path);
    static ModelConfig parse_yaml(std::string_view yaml_content);

    ~ModelConfig();

    // Specification Accessors
    [[nodiscard]] const ModelMetadata& metadata() const noexcept;
    [[nodiscard]] const TensorInputSpec& input_spec() const noexcept;
    [[nodiscard]] const TensorOutputSpec& output_spec() const noexcept;
    [[nodiscard]] const MitigationPolicyConfig& policy() const noexcept;

    // Validation
    [[nodiscard]] bool validate() const;

private:
    ModelConfig();
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace blackbox
```

---

## 2. Usage Example

```cpp
#include <blackbox/model_config.hpp>
#include <iostream>

void load_and_inspect() {
    auto config = blackbox::ModelConfig::load_from_file("/etc/blackbox/model_config.yaml");

    std::cout << "Model Name   : " << config.metadata().model_name << "\n"
              << "Input Shape  : [" << config.input_spec().dimensions[0] << ", " 
                                    << config.input_spec().dimensions[1] << "]\n"
              << "Drop Action  : " << config.policy().on_anomaly << "\n";
}
```

