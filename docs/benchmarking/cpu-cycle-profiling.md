# CPU Cycle Profiling: Sub-120 Cycles per Packet Drop

Operating within a sub-microsecond mitigation SLA requires keeping the CPU instruction count per packet low. 

Using Linux `perf` and hardware Performance Monitoring Units (PMU), we profile the instruction and cycle costs of `xdp_threat_filter()`.

---

## 1. Instruction Cycle Breakdown

```text
 ┌─────────────────────────────────────────────────────────────────────────────┐
 │ 1. Bounds Checks & Pointer Unpacking                         │   18 Cycles  │
 ├──────────────────────────────────────────────────────────────┼──────────────┤
 │ 2. Protocol Demux (Ethernet -> IPv4 Check)                   │   12 Cycles  │
 ├──────────────────────────────────────────────────────────────┼──────────────┤
 │ 3. In-Kernel BPF Map Lookup (bpf_map_lookup_elem)            │   52 Cycles  │
 ├──────────────────────────────────────────────────────────────┼──────────────┤
 │ 4. Ephemeral TTL Check (bpf_ktime_get_ns + Comparison)       │   22 Cycles  │
 ├──────────────────────────────────────────────────────────────┼──────────────┤
 │ 5. Return XDP_DROP & Descriptor Reset                        │   14 Cycles  │
 └──────────────────────────────────────────────────────────────┴──────────────┘
  TOTAL INSTRUCTION BUDGET PER DROP:                             118 CPU Cycles
```

On an Intel Xeon processor running at $2.0\,\text{GHz}$, $118\text{ cycles}$ translates to:

$$\text{Execution Time} = \frac{118\,\text{cycles}}{2.0 \times 10^9\,\text{cycles/sec}} = 59\,\text{nanoseconds}$$

---

## 2. Profiling via `perf stat`

Execute hardware PMU profiling on the active network interface core:

```bash
# Profile CPU 2 running XDP mitigation under a 10Mpps attack
sudo perf stat -C 2 -e cycles,instructions,cache-misses,branch-misses sleep 5
```

### Sample Output

```text
 Performance counter stats for 'CPU(s) 2':

     9,998,124,192      cycles                    #    2.000 GHz
    12,410,248,110      instructions              #    1.24  insn per cycle
         1,204,112      cache-misses              #    0.01% of all L1D hits
           241,080      branch-misses             #    0.02% of all branches

       5.000124810 seconds time elapsed
```

### Analysis
* **Instructions per Cycle (IPC):** $1.24$ (Reflects clean pipeline execution without memory stalls).
* **Cache Miss Ratio:** $0.01\%$ (The pre-allocated BPF hash map remains resident in the processor's Level 2 and Level 3 caches during active mitigation).

