# CMake Integration

> **Status:** Draft — placeholder content. Final technical prose is forthcoming.


Linking libblackbox.so into C++20 applications via CMake.

## System install

find_library + find_path, then link against pthread and libelf.

## Vendored

add_subdirectory(blackbox) exposes blackbox::blackbox with identical ABI.

```cmake
find_library(BLACKBOX_LIB blackbox REQUIRED PATHS /usr/local/lib)
find_path(BLACKBOX_INCLUDE_DIR blackbox/blackbox.hpp)
add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE ${BLACKBOX_LIB} pthread elf)
```

---

*Part of the blackbox-essential documentation set. See mkdocs.yml for navigation.*
