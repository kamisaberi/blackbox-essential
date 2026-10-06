# Anti-Cloning Defenses & Rogue Node Revocation

Virtualization and containerization make it easy for adversaries to duplicate active appliance disk images and spin up unauthorized, parallel clones to conduct side-channel attacks or bypass collective defense quotas.

`blackbox-essential` implements automated **Anti-Cloning Protections**.

---

## 1. The Appliance Cloning Threat

```text
 [ Production Appliance A (Valid) ] ──── Clone Image ────► [ Rogue Cloned Appliance B ]
  IP: 10.240.0.101                                           IP: 10.240.0.199
  Identical Certs, Disk Secrets, and Model Configurations
```

If an appliance relies solely on disk-stored certificates, the central orchestrator cannot distinguish Appliance A from Rogue Clone B.

---

## 2. Multi-Factor Anti-Cloning Detection Vectors

`blackbox::HardwareIdentity` monitors three independent factors to detect unauthorized duplication:

### 1. TPM Monotonic Command Counters (`TPM2_PT_TOTAL_COMMANDS`)
Physical and virtual TPM chips maintain internal, non-volatile monotonic counters that increment with every transaction:
$$\text{Counter}_{t+1} > \text{Counter}_t$$
If Nexus observes non-monotonic counter regressions (e.g., Appliance B reports a counter value lower than Appliance A's previous report), snapshot rollback or VM cloning is immediately flagged.

### 2. Ephemeral Nonce Challenge Windows
Nexus requires periodic cryptographic attestation quotes every $300\text{ seconds}$. Each challenge uses a unique random nonce with a **$2.0\text{ second}$ execution window**. Two cloned appliances sharing the same identity cannot answer challenges concurrently without colliding on nonce responses.

### 3. Hypervisor Boot UUID Verification
On VMware and KVM, the engine verifies the hypervisor's runtime UUID against cached bios measurements. If a VM clone is generated, ESXi generates a new virtual BIOS UUID (`uuid.bios`), causing an immediate mismatch against the TPM's measured PCR 0.

---

## 3. Automated Revocation Protocol

```text
 [ Nexus Fleet Hub Detects Clone / Nonce Collision ]
                         │
                         ▼ Emits Signed Revocation Command (gRPC 50051)
 ┌─────────────────────────────────────────────────────────────┐
 │ blackbox::HardwareIdentity::handle_revocation()             │
 └───────────────────────┬─────────────────────────────────────┘
                         │
        ┌────────────────┴────────────────┐
        ▼                                 ▼
 ┌───────────────┐                 ┌───────────────┐
 │ PURGE SECRETS │                 │ KERNEL LOCKOUT│
 └───────────────┘                 └───────────────┘
 Clears in-memory keys;            Commands xdp_filter.o to drop
 zeroizes model weights.           ALL network frames (Fail-Secure).
```

