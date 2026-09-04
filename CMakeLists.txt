cmake_minimum_required(VERSION 3.20)
project(blackbox VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# RPATH configuration
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "/usr/local/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Options
option(BLACKBOX_BUILD_TESTS "Build unit tests and benchmarks" ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/local/include
)

# System & External Dependencies
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_library(XINFER_LIB xinfer REQUIRED PATHS /usr/local/lib /usr/lib)
find_library(SQLITE3_LIB sqlite3 REQUIRED)

set(LIBBLACKBOX_SOURCES
    src/engine_instance.cpp
    src/hardware/hardware_identity.cpp
    src/ai/ai_manager.cpp
    src/ai/network_detector.cpp
    src/ai/vision_detector.cpp
    src/correlator/rules_engine.cpp
    src/correlator/threat_scorer.cpp
    src/mitigation/ebpf_blocker.cpp
    src/mitigation/process_killer.cpp
    src/mitigation/gpio_relay.cpp
    src/storage/ring_buffer.cpp
    src/storage/database.cpp
    src/ingest/network_ingest.cpp
    src/ingest/log_ingest.cpp
    src/ingest/vision_ingest.cpp
    src/api/http_server.cpp
    src/api/websocket.cpp
)

# -------------------------------------------------------------------
# BUILD SHARED LIBRARY (libblackbox.so)
# -------------------------------------------------------------------
add_library(blackbox SHARED ${LIBBLACKBOX_SOURCES})
add_library(blackbox::blackbox ALIAS blackbox)
target_compile_definitions(blackbox PRIVATE BLACKBOX_EXPORTS)

# Link bpf, elf, z directly as PUBLIC so dependencies propagate cleanly
target_link_libraries(blackbox PUBLIC
    bpf
    elf
    z
    Threads::Threads
    ${XINFER_LIB}
    ${SQLITE3_LIB}
    OpenSSL::Crypto
)

set_target_properties(blackbox PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "blackbox"
)

# -------------------------------------------------------------------
# DEMO DAEMON
# -------------------------------------------------------------------
add_executable(blackbox_daemon src/main.cpp)
target_link_libraries(blackbox_daemon PRIVATE 
    blackbox::blackbox 
    bpf 
    elf
)

if(BLACKBOX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation Rules
install(TARGETS blackbox DESTINATION /usr/local/lib)
install(DIRECTORY include/blackbox DESTINATION /usr/local/include)