# CMake Integration

Integrate `blackbox-essential` into external C++ applications using modern CMake targets.

---

## 1. Using `find_package` (Installed Shared Library)

When `libblackbox.so` is installed system-wide (via `sudo ninja install`), include it in your project's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)
project(EdgeMitigationAgent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Locate blackbox-essential package configuration
find_package(blackbox REQUIRED CONFIG)

add_executable(mitigation_agent
    src/main.cpp
    src/packet_hook.cpp
)

# Link against the core interface
target_link_libraries(mitigation_agent
    PRIVATE
        blackbox::blackbox
)

# Enforce high-performance optimization flags
target_compile_options(mitigation_agent PRIVATE -O3 -Wall -Wextra)
```

---

## 2. Using `FetchContent` (Direct Git Dependency)

If you prefer building `blackbox-essential` directly within your project's build tree without prior system installation:

```cmake
include(FetchContent)

FetchContent_Declare(
    blackbox_essential
    GIT_REPOSITORY https://github.com/kamisaberi/blackbox-essential.git
    GIT_TAG        v1.0.0
)

# Set dependency options before bringing it into scope
set(BLACKBOX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BLACKBOX_ENABLE_TPM ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(blackbox_essential)

add_executable(edge_agent src/main.cpp)
target_link_libraries(edge_agent PRIVATE blackbox::blackbox)
```

---

## 3. Required Linker Dependencies

`libblackbox.so` links against kernel ELF and TPM subsystems. If compiling directly without CMake, ensure the following flags are provided:

```bash
clang++-16 -std=c++20 main.cpp -o main \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lblackbox \
    -lbpf \
    -lelf \
    -ltss2-esys \
    -ltss2-rc \
    -Wl,-rpath,/usr/local/lib
```

