# Data Plane vs. Control Plane

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Fast-path kernel filter versus slow-path userspace controller.

## Data plane

XDP program + BPF maps: nanosecond verdicts, no context switches.

## Control plane

Policy compiler, telemetry, and OTA run in userspace and program the maps asynchronously.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
