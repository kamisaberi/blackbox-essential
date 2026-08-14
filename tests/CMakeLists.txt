add_executable(benchmark_siem benchmark_siem.cpp)
target_link_libraries(benchmark_siem PRIVATE xinfer ${SQLITE3_LIB} Threads::Threads)

add_executable(test_ai_manager test_ai_manager.cpp ../src/ai/ai_manager.cpp)
target_link_libraries(test_ai_manager PRIVATE xinfer Threads::Threads)

add_executable(test_correlator test_correlator.cpp ../src/correlator/rules_engine.cpp ../src/mitigation/ebpf_blocker.cpp)
target_link_libraries(test_correlator PRIVATE Threads::Threads)

add_executable(test_ingest test_ingest.cpp ../src/ingest/network_ingest.cpp)
target_link_libraries(test_ingest PRIVATE Threads::Threads)

add_executable(test_mitigation test_mitigation.cpp ../src/mitigation/ebpf_blocker.cpp)
target_link_libraries(test_mitigation PRIVATE Threads::Threads)