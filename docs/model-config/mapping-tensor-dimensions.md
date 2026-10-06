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

