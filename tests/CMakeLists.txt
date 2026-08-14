# CMake Test Configuration

enable_testing()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    /usr/local/include
)

# Shared Libraries
find_library(XINFER_LIB xinfer REQUIRED PATHS /usr/local/lib /usr/lib)
find_library(SQLITE3_LIB sqlite3 REQUIRED)

# Helper macro to create test binaries
macro(add_blackbox_test TEST_NAME SOURCE_FILE DEPENDENT_SOURCES)
    add_executable(${TEST_NAME} ${SOURCE_FILE} ${DEPENDENT_SOURCES})
    target_link_libraries(${TEST_NAME} PRIVATE
        ${XINFER_LIB}
        ${SQLITE3_LIB}
        Threads::Threads
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

# 1. Ingestion Drivers Unit Test
add_blackbox_test(test_ingest test_ingest.cpp 
    "../src/ingest/network_ingest.cpp;../src/ingest/log_ingest.cpp;../src/ingest/vision_ingest.cpp"
)

# 2. Autonomous Mitigation Engines Unit Test
add_blackbox_test(test_mitigation test_mitigation.cpp 
    "../src/mitigation/ebpf_blocker.cpp;../src/mitigation/process_killer.cpp;../src/mitigation/gpio_relay.cpp"
)

# 3. Correlation & Rules Engine Unit Test
add_blackbox_test(test_correlator test_correlator.cpp 
    "../src/correlator/rules_engine.cpp;../src/correlator/threat_scorer.cpp;../src/mitigation/ebpf_blocker.cpp"
)

# 4. xInfer Essential AI Engine Unit Test
add_blackbox_test(test_ai_manager test_ai_manager.cpp 
    "../src/ai/ai_manager.cpp;../src/ai/network_detector.cpp;../src/ai/vision_detector.cpp"
)

# 5. Microsecond SIEM End-to-End Latency Benchmark
add_executable(benchmark_siem benchmark_siem.cpp 
    ../src/ai/ai_manager.cpp
    ../src/correlator/rules_engine.cpp
    ../src/mitigation/ebpf_blocker.cpp
    ../src/storage/ring_buffer.cpp
)
target_link_libraries(benchmark_siem PRIVATE ${XINFER_LIB} ${SQLITE3_LIB} Threads::Threads)