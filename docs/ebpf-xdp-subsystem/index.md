# eBPF/XDP Subsystem

The eBPF/XDP subsystem forms the active in-kernel data plane of `blackbox-essential`. It evaluates ingress frames at wire speed inside network card driver rings.

## Section Navigation

- [XDP Filter Architecture](./xdp-filter-architecture.md): Frame ingress lifecycle in `xdp_threat_filter()` and return codes (`XDP_DROP`, `XDP_PASS`).
- [Driver Mode vs SKB Mode](./driver-mode-vs-skb-mode.md): Comparing Native Driver mode (`XDP_FLAGS_DRV_MODE`) with Generic SKB mode.
- [3GPP 5G GTP-U Slicing Guard](./3gpp-5g-gtp-u-slicing-guard.md): In-kernel N3 GTP-U dual-header parser & S-NSSAI TEID slice isolation guard.
- [Live-SBOM W^X Protection](./live-sbom-mprotect-wx.md): In-kernel tracepoint driver enforcing $W\oplus X$ memory invariants and $< 1.0\,\mu\text{s}$ `SIGKILL`.
- [BPF Verifier Guarantees](./bpf-verifier-guarantees.md): Memory safety proofs, bounds checking, and register state tracking.
- [Compiling BPF Bytecode](./compiling-bpf-bytecode.md): Clang/LLVM compilation pipeline, `build_bpf.sh`, and disassembly analysis.
- [BPF Map Management](./bpf-map-management.md): Pinned namespaces in `/sys/fs/bpf/blackbox` and `BPF_MAP_TYPE_HASH` mechanics.
- [Ephemeral Nanosecond TTL](./ttl-ephemeral-expiry.md): Monotonic `bpf_ktime_get_ns()` timestamp evaluation and user-space garbage collection.
- [Kernel Lockdown & Signing](./kernel-lockdown-and-signing.md): Cryptographic ELF signing via `sign-file` and TPM 2.0 PCR 7 measurements.
