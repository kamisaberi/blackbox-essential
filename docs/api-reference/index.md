# API Reference Overview

The `blackbox-essential` C++20 API provides a high-performance, deterministic programming interface for controlling eBPF kernel filters, lock-free ring buffers, and TPM 2.0 attestation.

## Section Navigation

- [API Reference Overview](./index.md): Header organization, RAII boundaries, and error design.
- [XdpManager](./xdp-manager.md): Class `blackbox::XdpManager` for BPF program loading and map mutation.
- [EventRingBuffer](./event-ring-buffer.md): Class `blackbox::EventRingBuffer` SPMC ring queue.
- [HardwareIdentity](./hardware-identity.md): Class `blackbox::HardwareIdentity` for TPM 2.0 quote generation.
- [ModelConfig](./model-config.md): Class `blackbox::ModelConfig` for YAML parsing and tensor binding.
- [KernelTelemetry](./kernel-telemetry.md): Struct `blackbox::KernelTelemetry` for per-CPU eBPF drop counters.
- [Data Structures](./data-structures.md): Common configuration structs (`XdpConfig`, `BlockedIpEntry`, `FlowEvent`).
- [Error Codes & Exceptions](./error-codes.md): Class `blackbox::BlackboxException` and `ErrorCode` enum.
