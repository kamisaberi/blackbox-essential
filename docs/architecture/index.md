# Architecture & Low-Level Design

`blackbox-essential` operates at the boundary between physical network silicon and user-space control daemons. This section details the decoupled 4-layer architecture, execution state machines, and microsecond physics.

## Section Navigation

- [Core Engine Design](./core-engine-design.md): 4-layer architecture, lifecycle state machine, and fast-path execution sequence.
- [Sub-Microsecond Physics](./sub-microsecond-physics.md): 10GbE wire speed mathematics, cycle budgets ($2{,}520$ cycles), and latency breakdown.
- [Zero-SKB Allocation](./zero-skb-allocation.md): Comparing `struct sk_buff` heap thrashing against zero-allocation `struct xdp_buff`.
- [Data Plane vs Control Plane](./data-plane-vs-control-plane.md): Decoupling in-kernel BPF execution from C++20 policy evaluation.
- [Kernel-Userspace ABI](./kernel-userspace-abi.md): ABI alignment rules, shared 64-bit structures (`abi.hpp`), and `bpf()` syscall interaction.
