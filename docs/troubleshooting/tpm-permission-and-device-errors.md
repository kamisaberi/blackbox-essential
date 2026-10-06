# TPM 2.0 Permission & Device Fault Diagnostics

This guide covers troubleshooting physical TPM 2.0 access issues, device node misconfigurations, and TCG TSS2 error codes.

---

## 1. `Failed to open /dev/tpmrm0: Permission denied (errno 13)`

### Symptom
```text
[Blackbox Fatal Error]
  Code       : -8 (ERR_TPM_INITIALIZATION_FAILED)
  Description: Esys_Initialize failed: could not open /dev/tpmrm0
  System Err : 13 (Permission denied)
```

### Cause
By default, the Linux TPM Resource Manager character device (`/dev/tpmrm0`) is owned by `root:tss` with permissions `0660`. The process executing `libblackbox.so` lacks permission to read and write to the device.

### Remediation
1. Add the operational service user to the `tss` system group:
   ```bash
   sudo usermod -aG tss $USER
   ```
2. Alternatively, deploy a dedicated udev rule to `/etc/udev/rules.d/70-tpmrm.rules`:
   ```udev
   KERNEL=="tpmrm[0-9]*", MODE="0666", GROUP="tss"
   ```
   Reload udev rules:
   ```bash
   sudo udevadm control --reload-rules && sudo udevadm trigger
   ```

---

## 2. `Device not found: /dev/tpmrm0`

### Cause
The physical host does not have a discrete TPM 2.0 chip, the TPM is disabled in the BIOS/UEFI settings, or the kernel TPM drivers are not loaded.

### Diagnosis Checklist
1. Inspect kernel boot messages:
   ```bash
   dmesg | grep -i tpm
   ```
2. If `tpm_crb` or `tpm_tis` errors appear, verify that **Intel PTT (Platform Trust Technology)** or **AMD fTPM** is set to **Enabled** in the motherboard BIOS settings.
3. If deploying inside a virtual machine, ensure that a **vTPM 2.0 Device** is explicitly attached to the virtual machine hardware configuration in VMware vSphere or Proxmox/KVM.

