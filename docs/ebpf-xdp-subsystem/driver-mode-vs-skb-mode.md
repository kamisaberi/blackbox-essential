# Native Driver Mode vs. Generic SKB Mode

`blackbox-essential` supports multiple XDP execution modes depending on network hardware, virtualization layers, and host driver capabilities.

---

## 1. Architectural Comparison

```text
NATIVE DRIVER MODE (XDP_FLAGS_DRV_MODE):
[ NIC Hardware DMA ] ──► [ Driver Rx Ring ] ──► [ xdp_filter.o ] ──► XDP_DROP (< 0.84 µs)
                                                       │
                                                       ▼ XDP_PASS
                                             [ alloc_skb() ] ──► Linux Network Stack

--------------------------------------------------------------------------------

GENERIC SKB MODE (XDP_FLAGS_SKB_MODE):
[ NIC Hardware DMA ] ──► [ Driver Rx Ring ] ──► [ alloc_skb() ]
                                                       │
                                                       ▼
                                            [ netif_receive_skb() ]
                                                       │
                                                       ▼
                                              [ xdp_filter.o ] ──► XDP_DROP (~2.5 - 5.0 µs)
                                                       │
                                                       ▼ XDP_PASS
                                             [ TCP/IP Stack ]
```

---

## 2. Mode Trade-Off Matrix

| Feature | Native Driver Mode (`DRV`) | Generic SKB Mode (`SKB`) | Hardware Offload (`HW`) |
| :--- | :--- | :--- | :--- |
| **Mitigation Latency** | **$< 0.84\,\mu\text{s}$** | $\sim 2.5 - 5.0\,\mu\text{s}$ | **$< 0.15\,\mu\text{s}$** |
| **`sk_buff` Allocated?** | **No** (Zero allocations) | Yes (Allocated prior to hook) | **No** |
| **Hardware Compatibility** | Intel, Mellanox, Broadcom | All Linux Network Devices | Netronome, SmartNICs |
| **Loopback / veth Support**| No | **Yes** | No |
| **Max Dropping Capacity** | **$14.88\text{ Mpps}$** | $\sim 1.8\text{ Mpps}$ | $> 50.0\text{ Mpps}$ |

---

## 3. Dynamic Mode Selection Logic in `XdpManager`

`blackbox-essential` selects the optimal mode automatically while allowing explicit programmatic overrides:

```cpp
#include <blackbox/xdp_manager.hpp>

blackbox::XdpConfig config;
config.interface_name = "eth0";
config.bpf_object_path = "/usr/local/lib/bpf/xdp_filter.o";

// Set attach policy
if (config.interface_name == "lo" || config.interface_name.starts_with("veth")) {
    // Virtual interfaces require SKB Generic mode
    config.attach_mode = blackbox::XdpAttachMode::SKB_GENERIC;
} else {
    // Physical NICs (ixgbe, mlx5, vmxnet3) utilize sub-microsecond native driver mode
    config.attach_mode = blackbox::XdpAttachMode::DRIVER;
}

blackbox::XdpManager xdp(config);
xdp.attach();
```

---

## 4. Querying Active Attachment Mode

Inspect the operational attachment mode using the `iproute2` suite:

```bash
ip link show dev eth0
```

### Native Driver Mode Output:
```text
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 xdp/id:42 ...
```

### Generic Mode Output:
```text
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 xdpgeneric/id:42 ...
```

