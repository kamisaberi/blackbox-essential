# blackbox::ModelConfig

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Declarative binding loader and threshold accessors.

## load()

Parses and schema-validates JSON/YAML bindings.

## thresholds()

Typed access to drop/learn/pass bands and block TTLs.

```cpp
auto cfg = blackbox::ModelConfig::load("models/v2.json");
float t = cfg.drop_threshold();  // 0.85
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
