# Tier 1: Physical TPM 2.0

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Interfacing with /dev/tpmrm0 via TCG TSS2 specifications.

## Flow

Open the resource manager, create an AIK, request PCR quotes over TSS2 ESAPI.

## Keys

Endorsement and attestation keys never leave silicon.

```bash
$ ls -l /dev/tpmrm0
$ tpm2_pcrread sha256:0,4
$ tpm2_quote -c aik.ctx -l sha256:0,4 -q $(cat nonce)
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
