# Benchmarking & Performance Evaluation

Empirical benchmark results, hardware testbed specifications, and comparative performance analyses.

## Section Navigation

- [Benchmarking Methodology](./methodology.md): Hardware testbed specifications, hardware timestamping, and environmental controls.
- [Latency Percentiles](./latency-percentiles.md): Empirical cumulative latency distributions ($p50 - p99.9$) and jitter analysis.
- [XDP vs. iptables/nftables](./xdp-vs-iptables-nftables.md): Comparative analysis between in-kernel XDP and Linux Netfilter firewalls.
- [XDP vs. Suricata NFQUEUE](./xdp-vs-suricata-nfqueue.md): Performance comparison between driver-level XDP and user-space NFQUEUE IPS daemons.
- [CPU Cycle Profiling](./cpu-cycle-profiling.md): Hardware PMU profiling demonstrating sub-120 CPU cycles per packet drop.
- [Memory Saturation Benchmarks](./memory-saturation-benchmarks.md): Ring buffer throughput scaling and 24-hour memory stability profiles.
