# blackbox::XdpManager

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Attach, rule injection, telemetry, and detach lifecycle.

## attach()

Binds xdp_filter.o to an interface in DRV or SKB mode.

## block_ip()

Inserts a TTL entry into blocked_ip_map; returns immediately.

```cpp
auto &xdp = blackbox::XdpManager::instance();
xdp.attach({.interface_name = "eth0"});
xdp.block_ip("198.51.100.45", 86'400'000'000'000ULL);
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
