# Benchmarking Methodology & Hardware Testbed Standards

Evaluating sub-microsecond in-kernel networking requires dedicated bare-metal hardware and hardware-level packet generation. Operating system virtualization, shared network switches, and unpinned user-space timers introduce measurement jitter that corrupts microsecond-level benchmarking.

---

## 1. Testbed Hardware Specifications

All empirical performance benchmarks documented for `blackbox-essential` were conducted on an isolated hardware testbed adhering to the following baseline:

```text
 ┌─────────────────────────────────────────────────────────────┐
 │                DUT (Device Under Test)                      │
 │  - System: Supermicro SYS-121H-TNR                          │
 │  - CPU: Dual Intel Xeon Platinum 8480+ (112 Cores, 2.0 GHz) │
 │  - RAM: 256 GB DDR5-4800 ECC Registered                    │
 │  - NIC: Intel E810-XXVDA2 (Dual-Port 25GbE SFP28)           │
 │  - OS: Ubuntu 24.04 LTS (Linux Kernel 6.8.0-31-generic)     │
 └──────────────────────────────▲──────────────────────────────┘
                                │ Direct Attach Copper (DAC) SFP28
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │             Hardware Traffic Generator (MoonGen / TRex)     │
 │  - System: Dell PowerEdge R750                              │
 │  - NIC: Intel E810-XXVDA2 (Dual-Port 25GbE SFP28)           │
 │  - Generator Engine: TRex v3.04 (Stateful & Stateless Mode) │
 │  - Timestamping: Hardware PTP IEEE 1588 Nanosecond Timers   │
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. Testing Standards & Invariants

1. **Packet Profile:** Minimum-sized Ethernet frames ($64\text{ bytes}$ frame payload + $20\text{ bytes}$ preamble/inter-frame gap = $84\text{ bytes}$ on the wire).
2. **Line-Rate Target:** $14.88\text{ Mpps}$ sustained on $10\text{ GbE}$ interfaces; $37.2\text{ Mpps}$ sustained on $25\text{ GbE}$ interfaces.
3. **Statistical Sample Size:** Latency and cycle measurements are captured over $N = 10{,}000{,}000$ consecutive packets.
4. **Hardware Timestamping:** Latency is measured by reading hardware ingress and egress timestamps directly from the Intel E810 NIC MAC/PHY layer, eliminating host OS timer distortion.

