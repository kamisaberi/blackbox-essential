#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <bpf/bpf_helpers.h>

// BPF Hash Map storing blocked IPv4 addresses (Key: IPv4 as u32, Value: Hit counter as u64)
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 65536);
    __type(key, __u32);
    __type(value, __u64);
} blocked_ip_map SEC(".maps");

SEC("xdp")
int xdp_firewall(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    // Parse Ethernet Header
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;

    if (eth->h_proto != __constant_htons(ETH_P_IP))
        return XDP_PASS;

    // Parse IP Header
    struct iphdr *ip = data + sizeof(struct ethhdr);
    if ((void *)(ip + 1) > data_end)
        return XDP_PASS;

    __u32 src_ip = ip->saddr;

    // Lookup IP in blocked BPF map
    __u64 *drop_counter = bpf_map_lookup_elem(&blocked_ip_map, &src_ip);
    if (drop_counter) {
        __sync_fetch_and_add(drop_counter, 1);
        return XDP_DROP; // Wire-speed nanosecond kernel drop
    }

    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";