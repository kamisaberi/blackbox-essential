# Error Codes

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


blackbox::BlackboxException and return-code taxonomy.

## Exceptions

Thrown for misuse: bad config, missing devices, failed attaches.

## Codes

Hot-path calls return Status enums; consult the table per call site.

```cpp
try {
  xdp.attach(cfg);   // throws on misuse
} catch (const blackbox::BlackboxException &e) {
  log(e.code(), e.what());
}
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
