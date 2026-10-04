# Packet Blaster Testing

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Stress-testing the SPMC ring with 1M+ packets/sec synthetic load.

## Tool

The bundled blaster replays PCAP at calibrated rates into a veth pair.

## Pass bar

Zero tail drops at 1M pps sustained for 60 seconds.

```bash
$ sudo ./build/pkt_blaster --pcap tests/mix.pcap --rate 1000000 --iface veth-test
[+] 60.0s @ 1.00Mpps, tail drops: 0
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
