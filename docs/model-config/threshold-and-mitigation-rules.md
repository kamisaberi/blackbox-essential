# Threshold & Mitigation Rules

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Mapping model output probabilities to in-kernel drop actions.

## Thresholds

Drop above 0.85, learn between 0.40–0.60, pass below. Per-model overrides allowed.

## TTL

Drops carry nanosecond TTLs, default 24h, into blocked_ip_map.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
