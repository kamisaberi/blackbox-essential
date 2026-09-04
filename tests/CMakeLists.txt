# tests/CMakeLists.txt

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    /usr/local/include
)

# 1. Ingest Drivers Test
add_executable(test_ingest test_ingest.cpp 
    ../src/ingest/network_ingest.cpp
    ../src/ingest/log_ingest.cpp
    ../src/ingest/vision_ingest.cpp
)
target_link_libraries(test_ingest PRIVATE blackbox::blackbox Threads::Threads)
add_test(NAME TestIngest COMMAND test_ingest)

# 2. Mitigation Engines Test (Added bpf elf z)
add_executable(test_mitigation test_mitigation.cpp 
    ../src/mitigation/ebpf_blocker.cpp
    ../src/mitigation/process_killer.cpp
    ../src/mitigation/gpio_relay.cpp
)
target_link_libraries(test_mitigation PRIVATE 
    blackbox::blackbox 
    bpf 
    elf 
    z 
    Threads::Threads
)
add_test(NAME TestMitigation COMMAND test_mitigation)

# 3. Correlator Test (Added bpf elf z)
add_executable(test_correlator test_correlator.cpp 
    ../src/correlator/rules_engine.cpp
    ../src/correlator/threat_scorer.cpp
    ../src/mitigation/ebpf_blocker.cpp
)
target_link_libraries(test_correlator PRIVATE 
    blackbox::blackbox 
    bpf 
    elf 
    z 
    Threads::Threads
)
add_test(NAME TestCorrelator COMMAND test_correlator)

# 4. AI Manager Test
add_executable(test_ai_manager test_ai_manager.cpp 
    ../src/ai/ai_manager.cpp
    ../src/ai/network_detector.cpp
    ../src/ai/vision_detector.cpp
)
target_link_libraries(test_ai_manager PRIVATE 
    blackbox::blackbox 
    ${XINFER_LIB} 
    Threads::Threads
)
add_test(NAME TestAIManager COMMAND test_ai_manager)

# 5. SIEM Latency Benchmark Binary
add_executable(benchmark_siem benchmark_siem.cpp)
target_link_libraries(benchmark_siem PRIVATE 
    blackbox::blackbox 
    bpf 
    elf 
    z 
    Threads::Threads
)