# BPF Verifier Rejection Diagnostics & Remediation

When compiling or loading `xdp_filter.o`, the Linux kernel BPF Verifier may reject the bytecode with a multi-page instruction trace. This guide translates common verifier errors into specific remediation steps.

---

## 1. `invalid access to packet, memptr=R1, offset=..., size=...`

### Verifier Trace Excerpt
```text
0: (61) r2 = *(u32 *)(r1 +4)
1: (61) r1 = *(u32 *)(r1 +0)
2: (71) r3 = *(u8 *)(r1 +14)
invalid access to packet, memptr=R1, offset=14, size=1
R1 min value is outside of the allowed memory range
```

### Cause
The code attempted to dereference packet memory (e.g., accessing Ethernet header byte 14 or the IP header) without first proving to the verifier that the pointer is strictly less than or equal to `ctx->data_end`.

### Remediation
Insert an explicit bounds check before the dereference:

```c
// Incorrect:
struct ethhdr *eth = data;
if (eth->h_proto == bpf_htons(ETH_P_IP)) { ... } // Verifier fails here!

// Correct:
struct ethhdr *eth = data;
if ((void *)(eth + 1) > data_end) {
    return XDP_PASS; // Bounds verified: eth + 14 bytes <= data_end
}
if (eth->h_proto == bpf_htons(ETH_P_IP)) { ... } // Permitted by verifier
```

---

## 2. `combined stack size of 4 frames is 544 bytes, stack limit 512 bytes`

### Cause
The eBPF execution model caps the total stack space at **512 bytes** across all call frames. Declaring large structs (such as a 256-byte telemetry buffer or array) on the stack triggers this failure.

### Remediation
Move large data structures off the stack and into a `BPF_MAP_TYPE_PERCPU_ARRAY` scratchpad map:

```c
// Incorrect:
struct large_flow_record record; // 384 bytes on stack -> FAILS!

// Correct:
__u32 zero = 0;
struct large_flow_record *record = bpf_map_lookup_elem(&scratchpad_map, &zero);
if (!record) return XDP_PASS;
// Use record safely in kernel memory
```

---

## 3. `back-edge from insn ... to ...` or `unbounded loop detected`

### Cause
Kernels prior to 5.3 strictly forbid back-edges (loops). Kernels 5.3+ permit bounded loops, but reject loops where the iteration count cannot be mathematically proven at compile time.

### Remediation
Force complete loop unrolling using the Clang unroll pragma:

```c
#pragma unroll
for (int i = 0; i < 4; ++i) {
    // Fixed, bounded unrolled execution path
}
```

