#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4
#define SIGKILL    9

// 1. Monitored Process ID Registry (Key: __u32 PID, Value: __u8 Enforcement Mode: 1=SIGKILL, 0=Audit)
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, __u32);
    __type(value, __u8);
} sbom_monitored_pids SEC(".maps");

// 2. Telemetry Statistics (Index: 0=Total mprotect, 1=PROT_EXEC Requests, 2=Killed Violations)
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 8);
    __type(key, __u32);
    __type(value, __u64);
} sbom_stats_map SEC(".maps");

// Generic tracepoint entry layout for sys_enter_mprotect
struct trace_event_raw_sys_enter_mprotect {
    struct trace_entry ent;
    long int id;
    unsigned long start;
    size_t len;
    unsigned long prot;
};

SEC("tracepoint/syscalls/sys_enter_mprotect")
int tracepoint_sys_enter_mprotect(struct trace_event_raw_sys_enter_mprotect *ctx) {
    __u64 pid_tgid = bpf_get_current_pid_tgid();
    __u32 pid = pid_tgid >> 32;

    // Track total mprotect calls
    __u32 key_total = 0;
    __u64 *total = bpf_map_lookup_elem(&sbom_stats_map, &key_total);
    if (total)
        __sync_fetch_and_add(total, 1);

    // Check if this process is monitored by Live-SBOM
    __u8 *mode = bpf_map_lookup_elem(&sbom_monitored_pids, &pid);
    if (!mode)
        return 0;

    unsigned long prot = ctx->prot;

    // Check for W^X Memory Invariant: Requesting Executable Permissions (PROT_EXEC)
    if (prot & PROT_EXEC) {
        __u32 key_exec = 1;
        __u64 *exec_cnt = bpf_map_lookup_elem(&sbom_stats_map, &key_exec);
        if (exec_cnt)
            __sync_fetch_and_add(exec_cnt, 1);

        // ENFORCEMENT: Active memory drift / shellcode injection detected
        if (*mode == 1) {
            __u32 key_killed = 2;
            __u64 *killed_cnt = bpf_map_lookup_elem(&sbom_stats_map, &key_killed);
            if (killed_cnt)
                __sync_fetch_and_add(killed_cnt, 1);

            // Sub-microsecond execution termination (< 1.0 µs)
            bpf_send_signal(SIGKILL);
        }
    }

    return 0;
}

char _license[] SEC("license") = "GPL";