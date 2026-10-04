# blackbox::KernelTelemetry

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Atomic counters: inspected, dropped, and mean SLA latency.

## Fields

total_packets_inspected, total_packets_dropped, mean_mitigation_latency_us.

## Cost

Relaxed atomics: readable at any rate with zero hot-path impact.

```cpp
auto s = xdp.get_telemetry();
// s.total_packets_inspected, s.total_packets_dropped,
// s.mean_mitigation_latency_us
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
