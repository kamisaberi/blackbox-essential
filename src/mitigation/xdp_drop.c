#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>
#include <bpf/bpf_core_read.h>

#define ETH_P_IP 0x0800
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define GTPU_PORT 2152
#define GTPU_G_PDU 0xFF

// 1. In-Kernel Drop Map (IPv4 -> Hit Counter)
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 65536);
    __type(key, __u32);
    __type(value, __u64);
} blocked_ip_map SEC(".maps");

// 2. AF_XDP Sockets Redirection Map (Rx Queue Index -> XSK Socket FD)
struct {
    __uint(type, BPF_MAP_TYPE_XSKMAP);
    __uint(max_entries, 64);
    __type(key, __u32);
    __type(value, __u32);
} xsks_map SEC(".maps");

// 3. Idea D: Deception Honeynet Target Specification
struct deception_target {
    __u32 decoy_ip;
    __u16 decoy_port;
    __u8  decoy_mac[6];
    __u8  rewrite_mac;
    __u8  pad;
    __u64 hit_counter;
};

// 4. In-Kernel Deception Redirection Map (Attacker IPv4 -> Decoy Target)
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 65536);
    __type(key, __u32);
    __type(value, struct deception_target);
} deception_map SEC(".maps");

// -----------------------------------------------------------------------------
// IDEA B: 5G/6G GTP-U & UPF NETWORK SLICING STRUCTURES
// -----------------------------------------------------------------------------

// 3GPP TS 29.281 GTP-U Header (8 bytes base)
struct gtpuhdr {
    __u8  flags;       // Version (3-bit), Protocol Type (1-bit), E, S, PN
    __u8  msg_type;    // 0xFF = G-PDU (Encapsulated user data)
    __be16 length;     // Payload length
    __be32 teid;       // 32-bit Tunnel Endpoint Identifier
} __attribute__((packed));

// 3GPP S-NSSAI Slice Policy
struct slice_policy {
    __u32 sst;               // 1=eMBB, 2=URLLC, 3=mMTC, 4=Enterprise
    __u32 sd;                // 24-bit Slice Differentiator
    __u32 allowed_inner_ip;  // 0 = wildcard/any
    __u16 allowed_ports[4];  // Authorized industrial ports (e.g. 502, 102)
    __u8  isolation_level;   // 0 = Strict, 1 = Permissive
    __u8  pad[3];
    __u64 packet_counter;
    __u64 violation_counter;
};

// 5. TEID -> 3GPP Slice Policy Map (Key: __u32 TEID)
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 65536);
    __type(key, __u32);
    __type(value, struct slice_policy);
} teid_slice_map SEC(".maps");

// 6. Per-Slice Drop & Violation Counters (Key: __u32 SST)
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 16);
    __type(key, __u32);
    __type(value, __u64);
} slice_drop_map SEC(".maps");

// Checksum Helpers
static __always_inline __u16 csum_fold_helper(__u32 sum) {
    #pragma unroll
    for (int i = 0; i < 4; i++) {
        if (sum >> 16)
            sum = (sum & 0xffff) + (sum >> 16);
    }
    return (__u16)~sum;
}

static __always_inline void update_csum32(__u16 *csum, __u32 old_val, __u32 new_val) {
    __u32 sum = (~bpf_ntohs(*csum) & 0xffff);
    __u16 old_hi = old_val >> 16;
    __u16 old_lo = old_val & 0xffff;
    __u16 new_hi = new_val >> 16;
    __u16 new_lo = new_val & 0xffff;
    sum += (~old_hi & 0xffff) + (~old_lo & 0xffff) + new_hi + new_lo;
    *csum = bpf_htons(csum_fold_helper(sum));
}

static __always_inline void update_csum16(__u16 *csum, __u16 old_val, __u16 new_val) {
    __u32 sum = (~bpf_ntohs(*csum) & 0xffff);
    sum += (~old_val & 0xffff) + new_val;
    *csum = bpf_htons(csum_fold_helper(sum));
}

SEC("xdp")
int xdp_firewall(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    // Parse Outer Ethernet
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;

    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;

    // Parse Outer IPv4
    struct iphdr *ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return XDP_PASS;

    __u32 src_ip = ip->saddr;

    // 1. Fast-path: Check blocked list (< 0.84 µs)
    __u64 *drop_counter = bpf_map_lookup_elem(&blocked_ip_map, &src_ip);
    if (drop_counter) {
        __sync_fetch_and_add(drop_counter, 1);
        return XDP_DROP;
    }

    // 2. Idea D Deception-path: Transparent Honeynet redirection
    struct deception_target *decoy = bpf_map_lookup_elem(&deception_map, &src_ip);
    if (decoy) {
        __sync_fetch_and_add(&decoy->hit_counter, 1);
        __u32 old_dst_ip = ip->daddr;
        __u32 new_dst_ip = decoy->decoy_ip;
        ip->daddr = new_dst_ip;
        update_csum32(&ip->check, old_dst_ip, new_dst_ip);

        if (ip->protocol == IPPROTO_TCP) {
            __u32 ip_hlen = ip->ihl * 4;
            if (ip_hlen >= sizeof(struct iphdr) && ip_hlen <= 60) {
                struct tcphdr *tcp = (void *)ip + ip_hlen;
                if ((void *)(tcp + 1) <= data_end) {
                    update_csum32(&tcp->check, old_dst_ip, new_dst_ip);
                    if (decoy->decoy_port != 0 && tcp->dest != decoy->decoy_port) {
                        __u16 old_port = tcp->dest;
                        tcp->dest = decoy->decoy_port;
                        update_csum16(&tcp->check, old_port, decoy->decoy_port);
                    }
                }
            }
        }
        if (decoy->rewrite_mac) {
            #pragma unroll
            for (int i = 0; i < 6; i++) {
                eth->h_dest[i] = decoy->decoy_mac[i];
            }
        }
        return XDP_PASS;
    }

    // -------------------------------------------------------------------------
    // 3. IDEA B: 5G/6G GTP-U & UPF NETWORK SLICING INSPECTION
    // -------------------------------------------------------------------------
    if (ip->protocol == IPPROTO_UDP) {
        __u32 outer_ip_hlen = ip->ihl * 4;
        if (outer_ip_hlen >= sizeof(struct iphdr) && outer_ip_hlen <= 60) {
            struct udphdr *udp = (void *)ip + outer_ip_hlen;
            if ((void *)(udp + 1) <= data_end) {
                if (udp->dest == bpf_htons(GTPU_PORT)) {
                    // Packet is GTP-U (N3 Interface Tunnel)
                    struct gtpuhdr *gtpu = (void *)(udp + 1);
                    if ((void *)(gtpu + 1) <= data_end && gtpu->msg_type == GTPU_G_PDU) {
                        __u32 teid = bpf_ntohl(gtpu->teid);

                        // Enforce Slice Policy Lookup in Kernel
                        struct slice_policy *policy = bpf_map_lookup_elem(&teid_slice_map, &teid);
                        if (!policy) {
                            // UNMAPPED / ROGUE TEID: Instant drop (Threat Vector 2)
                            __u32 rogue_key = 0;
                            __u64 *r_cnt = bpf_map_lookup_elem(&slice_drop_map, &rogue_key);
                            if (r_cnt)
                                __sync_fetch_and_add(r_cnt, 1);
                            return XDP_DROP;
                        }

                        // Determine offset of Inner IP Packet
                        // Base GTP-U is 8 bytes. If flags specify Extension/Seq/NPDU, header is 12+ bytes.
                        __u32 gtpu_hlen = sizeof(struct gtpuhdr);
                        if (gtpu->flags & 0x07) {
                            gtpu_hlen += 4;
                        }

                        void *inner_ip_start = (void *)gtpu + gtpu_hlen;
                        struct iphdr *inner_ip = inner_ip_start;

                        if ((void *)(inner_ip + 1) <= data_end && inner_ip->version == 4) {
                            __u32 inner_ip_hlen = inner_ip->ihl * 4;
                            if (inner_ip_hlen >= sizeof(struct iphdr) && inner_ip_hlen <= 60) {
                                void *inner_l4 = (void *)inner_ip + inner_ip_hlen;
                                __u16 inner_dport = 0;

                                if (inner_ip->protocol == IPPROTO_TCP) {
                                    struct tcphdr *inner_tcp = inner_l4;
                                    if ((void *)(inner_tcp + 1) <= data_end) {
                                        inner_dport = bpf_ntohs(inner_tcp->dest);
                                    }
                                } else if (inner_ip->protocol == IPPROTO_UDP) {
                                    struct udphdr *inner_udp = inner_l4;
                                    if ((void *)(inner_udp + 1) <= data_end) {
                                        inner_dport = bpf_ntohs(inner_udp->dest);
                                    }
                                }

                                // Slice Boundary Access Control Check
                                bool port_allowed = false;
                                #pragma unroll
                                for (int i = 0; i < 4; i++) {
                                    if (policy->allowed_ports[i] != 0 && policy->allowed_ports[i] == inner_dport) {
                                        port_allowed = true;
                                    }
                                }

                                // SLICE ISOLATION VIOLATION (e.g. eMBB accessing URLLC Modbus/S7 port)
                                if (!port_allowed && policy->isolation_level == 0) {
                                    __sync_fetch_and_add(&policy->violation_counter, 1);
                                    __u64 *s_cnt = bpf_map_lookup_elem(&slice_drop_map, &policy->sst);
                                    if (s_cnt)
                                        __sync_fetch_and_add(s_cnt, 1);
                                    return XDP_DROP; // Wire-speed slice violation drop (< 0.84 µs)
                                }

                                __sync_fetch_and_add(&policy->packet_counter, 1);
                            }
                        }
                    }
                }
            }
        }
    }

    // 4. Normal AF_XDP Zero-Copy userspace redirection
    return bpf_redirect_map(&xsks_map, ctx->rx_queue_index, XDP_PASS);
}

char _license[] SEC("license") = "GPL";