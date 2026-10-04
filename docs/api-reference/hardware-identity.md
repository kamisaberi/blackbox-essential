# blackbox::HardwareIdentity

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Tier probing, quote generation, and identity claims.

## probe()

Walks TPM → vTPM → DMI and returns the strongest available tier.

## quote()

Produces a signed attestation blob for enrollment frames.

```cpp
auto id = blackbox::HardwareIdentity::instance().probe();
log(id.machine_uuid, id.get_tier_string());
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
