# Quickstart Drop Test

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Five-minute first packet drop: minimal in-kernel filter test.

## Run it

Attach the smoke filter to a test veth pair, inject one blocked IP, watch the drop counter increment.

## Expected output

A single PASS line with measured attach-to-drop latency under 2.5µs in SKB mode.

```bash
$ sudo ./build/smoke_drop --iface veth-test --ip 198.51.100.45
[+] filter attached (SKB mode)
[+] drop verified in 1.9us — PASS
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
