# BPF Verifier Guarantees & Memory Bounds Safety Proofs

Before any eBPF program can be loaded into the Linux kernel, it must pass inspection by the in-kernel **BPF Verifier**. The verifier evaluates every possible instruction path, proving that the code cannot crash the operating system, access arbitrary memory, or enter an infinite loop.

---

## 1. The Core Verifier Guarantees

1. **Memory Safety:** The program can only read or write within explicitly bounded packet memory (`ctx->data` to `ctx->data_end`) or allocated map elements.
2. **Termination:** Unbounded loops are forbidden. All execution paths must reach an exit point within $1{,}000{,}000$ verified instructions.
3. **Type Safety:** Pointer types are strictly tracked. A pointer to a packet cannot be cast to an arbitrary kernel memory address.
4. **Stack Boundary:** The eBPF call frame stack is capped at **512 bytes**. Exceeding this boundary fails verification.

---

## 2. Mathematical Proof of Bounds Checking

Every packet memory access in `xdp_filter.c` is preceded by an explicit bounds check. The BPF verifier enforces the invariant that all offsets must satisfy:

$$\text{data} + \text{offset} + \text{sizeof}(\text{struct}) \le \text{data\_end}$$

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ ctx->data                                                   │
 └──────┬──────────────────────────────────────────────────────┘
        │
        ▼ eth = data
 ┌──────────────────────┐
 │ struct ethhdr (14B)  │
 └──────┬───────────────┘
        │  [ VERIFICATION PROOF: (eth + 1) <= data_end ]
        ▼ ip = eth + 1
 ┌──────────────────────┐
 │ struct iphdr (20B)   │
 └──────┬───────────────┘
        │  [ VERIFICATION PROOF: (ip + 1) <= data_end ]
        │  [ VERIFICATION PROOF: ((char *)ip + (ip->ihl * 4)) <= data_end ]
        ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ ctx->data_end                                               │
 └─────────────────────────────────────────────────────────────┘
```

If any memory dereference occurs without a prior bounds check, the verifier rejects the program at load time with an error:

```text
invalid access to packet, memptr=R1, offset=14, size=20
R1 min value is outside of the allowed memory range
```

---

## 3. Register State Tracking (R1 through R10)

The verifier tracks machine registers using abstract value types:

| Register | Purpose in `xdp_filter.o` |
| :--- | :--- |
| `R1` | First argument: Pointer to `struct xdp_md ctx` on entry. |
| `R2` - `R5` | Function call argument registers passed to BPF helpers. |
| `R0` | Function return value (e.g., result of `bpf_map_lookup_elem` or final `XDP_DROP`). |
| `R6` - `R9` | Callee-saved general-purpose registers (stores pointers to `data` and `data_end`). |
| `R10` | **Read-Only Frame Pointer:** References the 512-byte eBPF stack space. |

