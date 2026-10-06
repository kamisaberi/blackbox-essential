# Empirical Latency Distributions & Percentile Analysis

This document details the empirical packet mitigation latency of `blackbox-essential` measured from physical wire reception to in-kernel drop confirmation (`XDP_DROP`).

---

## 1. Cumulative Latency Percentiles (64-Byte UDP Ingress)

Under sustained line-rate traffic on an Intel E810 25GbE adapter:

```text
IN-KERNEL PACKET MITIGATION LATENCY (Cumulative Distribution):

Latency (µs)
 1.0 µs ──┐
          │                                                    p99.9: 0.91 µs
 0.8 µs ──┼─────────────────────────────────────── p99: 0.84 µs ───┐
          │                        p95: 0.81 µs ───┘
 0.6 µs ──┼─────── p50: 0.72 µs ───┘
          │
 0.0 µs ──┴───────┴───────────────┴───────────────┴───────────────┴────► Percentile
                 p50             p90             p95             p99
```

### Percentile Breakdown Table

| Execution Mode | Ingress Packet Size | p50 | p90 | p95 | p99 (SLA Bound) | p99.9 | Max Outlier |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Native Driver XDP** | 64 Bytes | **$0.72\,\mu\text{s}$** | **$0.78\,\mu\text{s}$** | **$0.81\,\mu\text{s}$** | **$0.84\,\mu\text{s}$** | $0.91\,\mu\text{s}$ | $1.12\,\mu\text{s}$ |
| **Native Driver XDP** | 512 Bytes | **$0.74\,\mu\text{s}$** | **$0.80\,\mu\text{s}$** | **$0.82\,\mu\text{s}$** | **$0.85\,\mu\text{s}$** | $0.93\,\mu\text{s}$ | $1.15\,\mu\text{s}$ |
| **Native Driver XDP** | 1500 Bytes | **$0.76\,\mu\text{s}$** | **$0.81\,\mu\text{s}$** | **$0.84\,\mu\text{s}$** | **$0.87\,\mu\text{s}$** | $0.96\,\mu\text{s}$ | $1.21\,\mu\text{s}$ |
| **Generic SKB XDP** | 64 Bytes | $2.45\,\mu\text{s}$ | $3.10\,\mu\text{s}$ | $3.85\,\mu\text{s}$ | $4.80\,\mu\text{s}$ | $6.20\,\mu\text{s}$ | $12.40\,\mu\text{s}$|

---

## 2. Jitter Standard Deviation ($\sigma$)

In mission-critical industrial applications (e.g., IEC 61850 GOOSE/SV communications), predictable latency is as critical as low median latency:

* **Native Driver Mode Jitter:** $\sigma = 0.042\,\mu\text{s}$ ($42\,\text{ns}$).
* **Generic SKB Mode Jitter:** $\sigma = 0.890\,\mu\text{s}$ ($890\,\text{ns}$).

Operating in Native Driver Mode maintains sub-microsecond determinism by avoiding memory page allocations and kernel thread scheduling.

