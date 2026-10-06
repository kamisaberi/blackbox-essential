# Configuring eBPF/XDP on VMware `ens33` / `vmxnet3` Virtual Interfaces

Running in-kernel XDP filters inside virtualized environments (such as **VMware Workstation, VMware Fusion, or VMware vSphere ESXi**) requires tuning virtual network adapter drivers to permit native driver execution.

---

## 1. VMware `vmxnet3` Driver Realities

The default virtual network adapter in enterprise VMware environments is **`vmxnet3`**. Linux kernel 5.15+ contains native XDP support for `vmxnet3`, but the driver defaults to enabling Large Receive Offload (LRO) and Generic Receive Offload (GRO), which conflicts with native XDP packet processing:

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ PROBLEM: Hardware Offloads (LRO / GRO) Enabled              │
 │  - Merges multiple incoming Ethernet frames into one large   │
 │    64 KB jumbo buffer inside the driver                     │
 │  - Breaks XDP invariant: 1 Descriptor == 1 Frame            │
 │  - Results in: "Operation not supported" (-EOPNOTSUPP)      │
 └──────────────────────────────┬──────────────────────────────┘
                                │
                                ▼ EXECUTE OFF-LOAD STRIPPING
 ┌─────────────────────────────────────────────────────────────┐
 │ SOLUTION: Disable LRO, GRO, and RX-VLAN Offloads via ethtool│
 │  - Restores strict single-frame descriptor boundaries        │
 │  - Enables sub-microsecond Native Driver XDP (XDP_DRV)      │
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. Step-by-Step Preparation Commands

Run these configuration commands on the VMware guest Linux OS prior to launching `blackbox-essential`:

```bash
# 1. Identify virtual interface name (e.g. ens33, ens160, or eth0)
IFACE="ens33"

# 2. Disable offloads that conflict with native XDP
sudo ethtool -K $IFACE lro off
sudo ethtool -K $IFACE gro off
sudo ethtool -K $IFACE rxvlan off
sudo ethtool -K $IFACE txvlan off

# 3. Increase ring buffer descriptors to avoid drops during bursts
sudo ethtool -G $IFACE rx 4096 tx 4096

# 4. Verify MTU is within standard bounds (<= 1500)
sudo ip link set dev $IFACE mtu 1500
```

---

## 3. C++ Code Configuration for VMware Interfaces

When writing initialization code for virtual environments, configure `XdpManager` to detect `vmxnet3` features automatically:

```cpp
#include <blackbox/xdp_manager.hpp>
#include <iostream>

blackbox::XdpConfig get_vmware_optimized_config(const std::string& iface_name) {
    blackbox::XdpConfig config;
    config.interface_name = iface_name;
    config.bpf_object_path = "/usr/local/lib/bpf/xdp_filter.o";
    config.max_blocked_ips = 32768;

    // Detect if running on virtual interface
    if (iface_name == "ens33" || iface_name == "ens160" || iface_name == "ens192") {
        std::cout << "[+] VMware virtual adapter detected. Selecting Native Driver mode.\n";
        config.attach_mode = blackbox::XdpAttachMode::DRIVER;
    } else {
        config.attach_mode = blackbox::XdpAttachMode::SKB_GENERIC;
    }

    return config;
}
```

---

## 4. Verification

Verify that the program attached in native driver mode:

```bash
ip link show dev ens33
```

### Expected Output
```text
2: ens33: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 xdp/id:142 ...
```

If `xdpgeneric` appears instead of `xdp`, ensure `gro` and `lro` are disabled via `ethtool -k ens33`.

