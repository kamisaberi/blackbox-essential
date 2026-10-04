# Sub-Microsecond Physics

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Why userspace firewalls fail critical cyber-physical systems.

## The race

A Modbus trip crosses the bus in 2–3.5ms; breakers move in 15–50ms. A 10ms verdict arrives after motion starts.

## The answer

Decide in the driver ring at 0.84µs — before sk_buff, before the stack, before userspace exists.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
