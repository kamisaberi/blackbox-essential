# Data Plane vs. Control Plane Architecture

`blackbox-essential` enforces an architectural boundary between its high-throughput **in-kernel Data Plane** and its flexible **user-space Control Plane**.

---

## 1. Architectural Separation

```text
 ┌─────────────────────────────────────────────────────────────┐
 │                CONTROL PLANE (User Space)                   │
 │                                                             │
 │  - Native ISO C++20 (libblackbox.so)                        │
 │  - blackbox::XdpManager                                     │
 │  - Telemetry Harvesting & Aggregation                       │
 │  - Ephemeral TTL Expiration Engine                          │
 │  - Model Inference & Policy Evaluation (Tier 1/Tier 3)      │
 │  - Non-Blocking System Calls (bpf(BPF_MAP_UPDATE_ELEM))     │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Bidirectional Shared Memory
                                │ (Pinned BPF Hash Maps)
 ┌──────────────────────────────┴──────────────────────────────┐
 │                 DATA PLANE (Kernel Space)                   │
 │                                                             │
 │  - In-Kernel eBPF Bytecode (xdp_filter.o)                   │
 │  - BPF JIT Compiled Native Machine Instructions             │
 │  - Hard Real-Time Execution Constraints (< 0.84 µs)         │
 │  - Zero System Calls, Zero Page Allocations                 │
 │  - Reads blocked_ip_map, Writes xdp_telemetry_map           │
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. Invariants & Responsibilities

| Attribute | Data Plane (eBPF Kernel) | Control Plane (C++20 User Space) |
| :--- | :--- | :--- |
| **Execution Environment** | Linux Kernel Driver Space | User Space (`libblackbox.so`) |
| **Timing Constraints** | Hard Real-Time ($< 0.84\,\mu\text{s}$) | Soft Real-Time ($< 50\,\text{ms}$) |
| **Memory Allocation** | Zero heap memory; stack bounded to 512B | Pre-allocated pinned pools & ring buffers |
| **Allowed Operations** | Arithmetic, bounds-checked packet reads | System calls, TPM quotes, telemetry aggregation |
| **Primary Task** | Immediate packet triage (`XDP_DROP` / `PASS`) | Threat decision, policy insertion, telemetry reporting |

---

## 3. Control-to-Data Plane Communication Channels

The Control Plane and Data Plane communicate through three BPF map structures:

### 1. `blocked_ip_map` (Policy Enforcement)
* **Type:** `BPF_MAP_TYPE_HASH`
* **Access Pattern:** 
  * Control Plane: Writes blocked IPv4 addresses with nanosecond expiration timestamps via `bpf_map_update_elem()`.
  * Data Plane: Performs lock-free lookups per incoming packet. If `current_time > expiry`, entry is ignored.

### 2. `telemetry_map` (Performance Tracking)
* **Type:** `BPF_MAP_TYPE_PERCPU_ARRAY`
* **Access Pattern:**
  * Data Plane: Atomically increments per-CPU drop and pass counters using lock-free primitives.
  * Control Plane: Queries and sums metrics every $1{,}000\,\text{ms}$ without interrupting kernel execution.

### 3. `event_ringbuf` (Flow Capture Tap)
* **Type:** `BPF_MAP_TYPE_RINGBUF`
* **Access Pattern:**
  * Data Plane: Streams packet headers and threat events to user-space workers for continuous AI scoring.

