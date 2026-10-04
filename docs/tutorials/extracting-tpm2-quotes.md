# Extracting TPM 2.0 Quotes

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Reading and verifying physical TPM 2.0 silicon quotes.

## Read

tpm2_quote over PCR 0 and 4 with a fresh nonce.

## Verify

Check the AIK signature and compare digests to golden values.

```bash
$ tpm2_createak -c aik.ctx
$ tpm2_quote -c aik.ctx -l sha256:0,4 -q $NONCE -m quote.msg -s quote.sig
$ tpm2_checkquote -u aik.pub -m quote.msg -s quote.sig -f golden.pcr
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
