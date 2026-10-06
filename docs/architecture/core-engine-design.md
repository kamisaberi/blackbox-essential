# Decoupled Tier 2 Architecture & Execution Pipeline

`blackbox-essential` (`libblackbox.so`) serves as the foundational active mitigation core within the Aryorithm ecosystem. It sits between hardware networking silicon (Tier 1/NIC) and higher-level cyber-physical XDR daemons (Tier 3 `blackbox-sentinel`), ensuring that threat mitigation decisions execute with sub-microsecond determinism.

---

## 1. Subsystem Architecture

The library is decoupled into four isolated architectural layers:

```text
+─────────────────────────────────────────────────────────────────────────────+
|               Layer 4: C++20 Control Plane & Orchestrator                   |
|    - blackbox::XdpManager             - blackbox::HardwareIdentity          |
|    - blackbox::ModelConfig            - Ephemeral TTL Eviction Scanner      |
+─────────────────────────────────────────────────────────────────────────────+
                                       │
                    ┌──────────────────┴──────────────────┐
                    ▼                                     ▼
+───────────────────────────────────────+ +───────────────────────────────────+
| Layer 3: Lock-Free Concurrency Core   | | Layer 2: Silicon Root of Trust    |
| - SPMC EventRingBuffer (1.25M EPS)    | | - Physical TPM 2.0 PCR Quotes     |
| - Cache-Aligned Atomic Head/Tail      | | - VMware vTPM Attestation         |
| - Non-Blocking Driver Enqueue         | | - Motherboard DMI SHA-256 Binding |
+───────────────────────────────────────+ +───────────────────────────────────+
                    │                                     │
                    └──────────────────┬──────────────────┘
                                       │
                                       ▼
+─────────────────────────────────────────────────────────────────────────────+
|               Layer 1: In-Kernel Fast-Path Filter (xdp_filter.o)            |
|    - Driver-Space XDP Hook            - BPF Map: blocked_ip_map             |
|    - Nanosecond In-Kernel Drop Gate   - Per-CPU Packet/Byte Telemetry Maps  |
+─────────────────────────────────────────────────────────────────────────────+
```

---

## 2. Engine Lifecycle State Machine

The lifecycle of `libblackbox.so` transitions through five formal states to guarantee that network driver rings are never left in an unhandled condition:

```text
 ┌─────────────────┐
 │   UNINITIALIZED │
 └────────┬────────┘
          │ configure(XdpConfig)
          ▼
 ┌─────────────────┐
 │   CONFIGURED    │
 └────────┬────────┘
          │ attach() -> Validates BTF, loads xdp_filter.o, binds driver ring
          ▼
 ┌─────────────────┐
 │     ARMED       │◄─────────────────────────────┐
 └────────┬────────┘                              │
          │ block_ip() / Traffic Ingress          │ Passive filtering (XDP_PASS)
          ▼                                       │
 ┌─────────────────┐                              │
 │  ACTIVE_DROP    │──────────────────────────────┘
 └────────┬────────┘
          │ detach() / Process Termination (SIGTERM / RAII)
          ▼
 ┌─────────────────┐
 │   TERMINATED    │
 └─────────────────┘
```

### State Transitions & Safety Invariants

| State | Permitted Transitions | Invariants Enforced |
| :--- | :--- | :--- |
| `UNINITIALIZED` | `CONFIGURED` | Zero kernel hooks; zero pinned BPF maps. |
| `CONFIGURED` | `ARMED`, `TERMINATED` | Interface existence verified; MTU bounds validated. |
| `ARMED` | `ACTIVE_DROP`, `TERMINATED` | eBPF program loaded; telemetry map pinned; default action `XDP_PASS`. |
| `ACTIVE_DROP` | `ARMED`, `TERMINATED` | Packets matching `blocked_ip_map` drop in $< 0.84\,\mu\text{s}$ at driver hook. |
| `TERMINATED` | `UNINITIALIZED` | XDP program unhooked from netdev; map file descriptors closed; ring drained. |

---

## 3. Fast-Path Packet Processing Sequence

When an incoming Ethernet frame arrives at the physical physical coding sublayer (PCS):

1. **DMA Reception:** The NIC deposits the raw Ethernet frame directly into host RAM via Direct Memory Access (DMA).
2. **Driver XDP Trigger:** Before allocating socket buffers (`sk_buff`), the network driver invokes `xdp_threat_filter()`.
3. **Parse Headers:** The eBPF program bounds-checks the frame and extracts protocol headers (IPv4/IPv6, TCP/UDP).
4. **Hash Table Lookup:** The source IPv4 address is looked up in `blocked_ip_map`.
5. **Mitigation Decision:**
   * **Hit:** Returns `XDP_DROP` immediately ($< 0.84\,\mu\text{s}$). Driver recycles the RX descriptor back to the hardware ring.
   * **Miss:** Returns `XDP_PASS`. Packet proceeds to the Linux kernel TCP/IP networking stack.

