# Installation

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Building from source, package managers, and library installation.

## From source

Build the bytecode first, then the userspace library — order matters.

## Packages

.deb/.rpm per release; air-gapped .snbundle archives for classified sites.

```bash
$ git clone https://github.com/kamisaberi/blackbox.git
$ ./bpf/build_bpf.sh
$ cmake -S blackbox -B build -DCMAKE_BUILD_TYPE=Release
$ cmake --build build -j$(nproc)
$ sudo cmake --install build
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
