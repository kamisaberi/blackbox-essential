# API Reference Overview

The `blackbox-essential` C++20 API provides a high-performance, deterministic programming interface for configuring Linux eBPF/XDP kernel filters, managing lock-free Single-Producer Multi-Consumer (SPMC) ring buffers, and generating cryptographic TPM 2.0 hardware attestation claims.

---

## 1. Header Organization

Include the primary umbrella header to access all core capabilities:

```cpp
#include <blackbox/blackbox.hpp>
```

Alternatively, include modular headers for granular compilation:

| Header File | Primary Declarations |
| :--- | :--- |
| `<blackbox/xdp_manager.hpp>` | Class `blackbox::XdpManager`, `XdpConfig`, `XdpAttachMode` |
| `<blackbox/event_ring_buffer.hpp>` | Class `blackbox::EventRingBuffer`, `FlowEvent` |
| `<blackbox/hardware_identity.hpp>` | Class `blackbox::HardwareIdentity`, `IdentityClaims`, `IdentityTier` |
| `<blackbox/model_config.hpp>` | Class `blackbox::ModelConfig`, `DynamicTensorBinder` |
| `<blackbox/telemetry.hpp>` | Struct `blackbox::KernelTelemetry`, `RingBufferMetrics` |
| `<blackbox/types.hpp>` | Common types, `BlockedIpEntry`, network byte order helpers |
| `<blackbox/exception.hpp>` | Class `blackbox::BlackboxException`, enum `ErrorCode` |

---

## 2. Invariants & Design Principles

1. **Explicit Resource Boundaries (RAII):** Attaching to network drivers and binding TPM contexts acquire persistent system resources. Destructors guarantee that eBPF programs are cleanly unhooked and hardware session handles are released.
2. **Zero-Copy Interoperability:** Network payloads, telemetry frames, and tensor views interface via non-owning `std::span` buffers, preventing heap churn on the critical path.
3. **Deterministic Error Handling:** Methods on the microsecond mitigation path use `noexcept` specifications and return status codes, while initialization and configuration routines raise strongly-typed `BlackboxException` instances on unrecoverable failures.

