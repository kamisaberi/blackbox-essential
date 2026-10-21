#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>
#include <bpf/bpf_core_read.h>

#define ETH_P_IP 0x0800
#define IPPROTO_TCP 6

// 1. In-Kernel Drop Map (IPv4 -> Hit Counter)
struct
{
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 65536);
    __type(key, __u32);
    __type(value, __u64);
} blocked_ip_map SEC(".maps");

// 2. AF_XDP Sockets Redirection Map (Rx Queue Index -> XSK Socket FD)
struct
{
    __uint(type, BPF_MAP_TYPE_XSKMAP);
    __uint(max_entries, 64);
    __type(key, __u32);
    __type(value, __u32);
} xsks_map SEC(".maps");

// 3. Idea D: Deception Honeynet Target Specification
struct deception_target
{
    __u32 decoy_ip;    // Decoy Target IPv4 in network byte order
    __u16 decoy_port;  // Decoy TCP Port in network byte order (0 = preserve)
    __u8 decoy_mac[6]; // Decoy Destination MAC (optional L2 redirect)
    __u8 rewrite_mac;  // Flag: 1 = apply decoy_mac, 0 = keep MAC
    __u8 pad;
    __u64 hit_counter; // Telemetry counter for intercepted packets
};

// 4. In-Kernel Deception Redirection Map (Attacker IPv4 -> Decoy Target)
struct
{
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 65536);
    __type(key, __u32);
    __type(value, struct deception_target);
} deception_map SEC(".maps");

// Helper: RFC 1624 Incremental 16-bit 1's Complement Checksum Fold
static __always_inline __u16 csum_fold_helper(__u32 sum)
{
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return (__u16)~sum;
}

// Helper: Update 16-bit Checksum for 32-bit delta (e.g. IPv4 addresses)
static __always_inline void update_csum32(__u16 *csum, __u32 old_val, __u32 new_val)
{
    __u32 sum = (~bpf_ntohs(*csum) & 0xffff);
    __u16 old_hi = old_val >> 16;
    __u16 old_lo = old_val & 0xffff;
    __u16 new_hi = new_val >> 16;
    __u16 new_lo = new_val & 0xffff;

    sum += (~old_hi & 0xffff) + (~old_lo & 0xffff) + new_hi + new_lo;
    *csum = bpf_htons(csum_fold_helper(sum));
}

// Helper: Update 16-bit Checksum for 16-bit delta (e.g. TCP ports)
static __always_inline void update_csum16(__u16 *csum, __u16 old_val, __u16 new_val)
{
    __u32 sum = (~bpf_ntohs(*csum) & 0xffff);
    sum += (~old_val & 0xffff) + new_val;
    *csum = bpf_htons(csum_fold_helper(sum));
}

SEC("xdp")
int xdp_firewall(struct xdp_md *ctx)
{
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    // Parse Ethernet Header
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;

    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;

    // Parse IPv4 Header
    struct iphdr *ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return XDP_PASS;

    __u32 src_ip = ip->saddr;

    // 1. Hard Drop Fast-path: Check blocked list
    __u64 *drop_counter = bpf_map_lookup_elem(&blocked_ip_map, &src_ip);
    if (drop_counter)
    {
        __sync_fetch_and_add(drop_counter, 1);
        return XDP_DROP; // Wire-speed nanosecond kernel drop (< 0.84 µs)
    }

    // 2. Idea D Deception-path: Check if attacker is marked for Honeynet trapping
    struct deception_target *decoy = bpf_map_lookup_elem(&deception_map, &src_ip);
    if (decoy)
    {
        __sync_fetch_and_add(&decoy->hit_counter, 1);

        __u32 old_dst_ip = ip->daddr;
        __u32 new_dst_ip = decoy->decoy_ip;

        // Perform in-kernel IP destination rewrite (DNAT to Honeynet)
        ip->daddr = new_dst_ip;
        update_csum32(&ip->check, old_dst_ip, new_dst_ip);

        // If TCP, perform TCP pseudo-header & port rewrite
        if (ip->protocol == IPPROTO_TCP)
        {
            // Verify IP header length bounds before indexing TCP header
            if (ip->ihl < 5)
                return XDP_PASS;

            struct tcphdr *tcp = (void *)ip + (ip->ihl * 4);
            if ((void *)(tcp + 1) <= data_end)
            {
                // Update TCP checksum for the destination IP change in pseudo-header
                update_csum32(&tcp->check, old_dst_ip, new_dst_ip);

                // Optional port rewrite
                if (decoy->decoy_port != 0 && tcp->dest != decoy->decoy_port)
                {
                    __u16 old_port = tcp->dest;
                    tcp->dest = decoy->decoy_port;
                    update_csum16(&tcp->check, old_port, decoy->decoy_port);
                }
            }
        }

        // Optional L2 MAC rewrite for direct physical forwarding to honeynet node
        if (decoy->rewrite_mac)
        {
#pragma unroll
            for (int i = 0; i < 6; i++)
            {
                eth->h_dhost[i] = decoy->decoy_mac[i];
            }
        }

        // Return XDP_PASS: packet continues through Linux stack routed directly to the decoy container
        return XDP_PASS;
    }

    // 3. Normal AF_XDP Zero-Copy userspace redirection
    return bpf_redirect_map(&xsks_map, ctx->rx_queue_index, XDP_PASS);
}

char _license[] SEC("license") = "GPL";