# XDP on VMware vNICs

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Configuring eBPF on VMware ens33 / vmxnet3 virtual interfaces.

## Mode

Force SKB mode; verify with the probe tool before attaching.

## Expectations

Sub-2.5µs verdicts; full map and ring semantics unchanged.

```bash
$ ./build/xdp_probe --iface ens33   # expect: SKB mode
$ sudo ./build/smoke_drop --iface ens33 --ip 198.51.100.45
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
