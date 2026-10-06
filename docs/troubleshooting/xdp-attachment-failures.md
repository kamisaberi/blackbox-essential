# XDP Driver Attachment Failures

This guide resolves errors encountered when invoking `XdpManager::attach()` or attaching `xdp_filter.o` via `ip link`.

---

## 1. `Operation not supported (-EOPNOTSUPP)`

### Symptom
```text
[Blackbox Fatal Error]
  Code       : -1 (ERR_XDP_ATTACH_FAILED)
  Description: Failed to attach XDP program to interface eth0
  System Err : 95 (Operation not supported)
```

### Causes & Fixes
1. **Conflicting Offloads (LRO/GRO):** Native Driver XDP requires Large Receive Offload (LRO) and Generic Receive Offload (GRO) to be disabled.
   ```bash
   sudo ethtool -K eth0 lro off gro off rxvlan off txvlan off
   ```
2. **MTU Exceeds Driver Page Bounds:** Many NIC drivers restrict XDP to standard MTUs ($\le 1500$). If jumbo frames are enabled:
   ```bash
   sudo ip link set dev eth0 mtu 1500
   ```
3. **Driver Lacks Native XDP Support:** If using a legacy adapter (e.g., `e1000e`), native driver mode is unsupported. Fall back to generic mode:
   ```cpp
   config.attach_mode = blackbox::XdpAttachMode::SKB_GENERIC;
   ```

---

## 2. `Device or resource busy (-EBUSY)`

### Symptom
```text
System Err: 16 (Device or resource busy)
```

### Cause
Another XDP program or security agent (such as Cilium, Cloudflare `bpftools`, or a previously crashed instance of `libblackbox.so`) is already attached to the network interface.

### Remediation
Force-detach any active XDP program before attaching:

```bash
# Check active attachments
ip link show dev eth0

# Detach any existing native or generic XDP program
sudo ip link set dev eth0 xdp off
sudo ip link set dev eth0 xdpgeneric off
```

Then restart `blackbox-essential`.

