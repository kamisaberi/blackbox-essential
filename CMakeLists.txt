cmake_minimum_required(VERSION 3.20)
project(blackbox VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Compiler Flags for High Performance on Linux/Ubuntu
add_compile_options(-O3 -Wall -Wextra -pthread -march=native)

# Include Directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/local/include
)

# Find System Dependencies on Ubuntu
find_package(Threads REQUIRED)
find_library(XINFER_LIB xinfer REQUIRED PATHS /usr/local/lib /usr/lib)
find_library(SQLITE3_LIB sqlite3 REQUIRED)

# Core Source Files
set(BLACKBOX_SOURCES
    src/main.cpp
    src/ai/ai_manager.cpp
    src/correlator/rules_engine.cpp
    src/mitigation/ebpf_blocker.cpp
)

# Executable Binary
add_executable(blackbox ${BLACKBOX_SOURCES})

# Link Libraries
target_link_libraries(blackbox PRIVATE
    Threads::Threads
    ${XINFER_LIB}
    ${SQLITE3_LIB}
)

# Installation
install(TARGETS blackbox DESTINATION /usr/local/bin)