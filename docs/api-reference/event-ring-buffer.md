# blackbox::EventRingBuffer

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


SPMC construction, publish/claim semantics, and capacity policy.

## Construct

Power-of-two capacity; 262,144 is the reference deployment.

## Publish/claim

Wait-free publish; CAS-competing claims; full ring applies backpressure.

```cpp
blackbox::EventRingBuffer ring(262144);
ring.publish(std::move(event));  // never blocks
Event e; ring.claim(e);            // CAS claim
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
