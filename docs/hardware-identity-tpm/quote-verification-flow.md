# Quote Verification Flow

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Attestation Identity Key (AIK) signature validation protocol.

## Steps

Fetch quote → check nonce freshness → verify AIK signature → compare PCRs to golden values.

## Freshness

Nonces expire in 60 seconds; replays die on arrival.

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
