# VMware & Virtual Ethernet (`veth`) Edge Cases

Running in-kernel XDP filters across virtualized environments (such as Docker-in-VMware or multi-homed ESXi hosts) introduces virtual switch and packet delivery quirks.

---

## 1. Virtual Ethernet Pairs (`veth`) Fail in Driver Mode

### Symptom
Attaching an XDP program to a Docker container's `veth` interface fails with `Operation not supported`.

### Cause
Virtual Ethernet pair drivers (`veth`) do not have physical hardware DMA descriptors; they simulate network delivery via standard kernel memory buffers. 

### Remediation
Always configure virtual pairs with `SKB_GENERIC` mode:

```cpp
if (interface_name.starts_with("veth") || interface_name.starts_with("docker")) {
    config.attach_mode = blackbox::XdpAttachMode::SKB_GENERIC;
}
```

---

## 2. Packets Dropping Silently on VMware ESXi vSwitch

### Symptom
XDP programs attach without errors on VMware guests running `vmxnet3`, but ingress packets never reach the filter or drop counters remain zero during traffic floods.

### Cause
The VMware vSphere Virtual Switch (vSwitch) or Distributed Port Group blocks forged packet headers and drops promiscuous frames before they reach the virtual machine's vNIC.

### Remediation
In VMware vSphere / ESXi host settings, edit the Port Group security policy:

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ VMware vSwitch / Port Group Security Policies               │
 ├─────────────────────────────────────────────────────────────┤
 │ Promiscuous Mode            : ACCEPT                        │
 │ MAC Address Changes         : ACCEPT                        │
 │ Forged Transmits            : ACCEPT                        │
 └─────────────────────────────────────────────────────────────┘
```

Set all three policies to **Accept** to allow raw packet flows to reach the `vmxnet3` driver.

