# VMware & veth SKB Issues

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Debugging packet drops on VMware virtual interfaces and veth pairs.

## Symptoms

Silent drops with healthy counters usually mean empty FILL rings.

## Fixes

Grow descriptors, confirm SKB mode engaged, check vSwitch promiscuous policy.

```bash
$ ethtool -i ens33   # driver must list XDP (even SKB)
$ ./build/ring_stat --iface ens33
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
