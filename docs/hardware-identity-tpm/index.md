# Hardware Identity & TPM 2.0 Attestation

`blackbox::HardwareIdentity` cryptographically binds each running node to a physical silicon root of trust, preventing software credential theft and image cloning.

## Section Navigation

- [Attestation Architecture](./attestation-architecture.md): Vulnerabilities of software identities and the 3-tier identity hierarchy.
- [Tier 1 Physical TPM 2.0](./tier1-physical-tpm2.md): Interfacing with physical cryptoprocessors via `/dev/tpmrm0` and TSS2 ESYS.
- [PCR Measurements & Quotes](./tpm2-pcr-measurements.md): Generating hardware signed quotes over PCR 0 (BIOS) and PCR 4 (Bootloader).
- [Tier 2 Virtual TPM](./tier2-virtual-tpm.md): Virtualized TPM 2.0 (vTPM) integration in VMware vSphere and KVM environments.
- [Tier 3 DMI Fallback](./tier3-dmi-fallback.md): SHA-256 fallback identity hashing for legacy DIN-rail IPCs.
- [Anti-Cloning Protections](./anti-cloning-protections.md): Monotonic command counter tracking, nonce challenge windows, and automated revocation.
- [Quote Verification Flow](./quote-verification-flow.md): Complete cryptographic handshake protocol between edge nodes and `sentinel-nexus`.
