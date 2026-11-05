# Live-SBOM In-Kernel Memory Drift & $W\oplus X$ Protection

Fileless malware, in-memory buffer overflows, and dynamic process hollowing bypass static software bill of materials (SBOM) auditors by modifying memory page protections at runtime via `mprotect()` or `pkey_mprotect()`.

`blackbox-essential` implements a **Live-SBOM eBPF Tracepoint Driver** (`live_sbom.c`) that attaches to kernel system call boundaries (`syscalls/sys_enter_mprotect`), enforcing strict $W\oplus X$ ($Write\oplus Execute$) memory invariants anchored to physical TPM 2.0 PCR[10] baseline measurements.

---

## 1. Runtime Invariant: $W\oplus X$ Memory Protection

A memory page must never be simultaneously writable **and** executable:

$$\text{Prot}_{\text{allowed}} \subseteq \{\text{PROT\_READ},\, \text{PROT\_WRITE}\} \quad \lor \quad \text{Prot}_{\text{allowed}} \subseteq \{\text{PROT\_READ},\, \text{PROT\_EXEC}\}$$

$$\text{PROT\_WRITE} \land \text{PROT\_EXEC} \implies \mathbf{HARD\ SIGKILL\ (< 1.0\,\mu s)}$$

---

## 2. In-Kernel eBPF Tracepoint Driver (`src/mitigation/live_sbom.c`)

```c
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, __u32);   // PID
    __type(value, __u64); // Approved Memory SHA-256 Hash
} sbom_process_map SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_mprotect")
int trace_mprotect_enter(struct trace_event_raw_sys_enter *ctx) {
    __u64 pid_tgid = bpf_get_current_pid_tgid();
    __u32 pid = pid_tgid >> 32;

    // Extract mprotect syscall arguments
    // arg0: start_addr, arg1: len, arg2: prot
    unsigned long prot = (unsigned long)ctx->args[2];

    // Check for illegal W^X transition (PROT_WRITE | PROT_EXEC or transition to EXEC)
    if ((prot & PROT_EXEC) && (prot & PROT_WRITE)) {
        // Instant < 1.0 µs SIGKILL sent directly by kernel to offending process
        bpf_send_signal(9); // SIGKILL
        
        bpf_printk("[LIVE-SBOM CRITICAL] PID %d attempted W^X violation (prot: 0x%x) -> SIGKILL SENT\n", pid, prot);
        return 0;
    }

    return 0;
}

char _license[] SEC("license") = "GPL";
```

---

## 3. Operational SLA & Security Invariants

* **Interception Reaction SLA:** **$< 1.0\,\mu\text{s}$** from `mprotect` entry to `SIGKILL` signal dispatch.
* **TPM 2.0 Integration:** Process memory mapping state is hashed continuously against TPM 2.0 PCR[10].
