# BPF Verifier Rejections

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Debugging R1 invalid mem access, unbounded loops, and stack size errors.

## Classic

Stale pointers after adjust_head; unaligned packet reads; missing bounds checks.

## Playbook

Reload pointers, check-then-touch every header, memcpy into locals.

```bash
$ bpftool prog load xdp.o /sys/fs/bpf/xdp 2>&1 | head -30
# read the first error only — later ones cascade
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
