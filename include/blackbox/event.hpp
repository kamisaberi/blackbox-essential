#pragma once
#include <string>
#include <vector>
#include <chrono>
#include "types.hpp"

namespace blackbox {

struct SecurityEvent {
    uint64_t event_id;
    std::chrono::system_clock::time_point timestamp;
    EventType type;
    
    std::string source_ip;
    std::string destination_ip;
    uint16_t port{0};
    
    float anomaly_score{0.0f};
    ThreatLevel level{ThreatLevel::Info};
    ActionType action_taken{ActionType::None};
    
    std::string description;
    std::vector<float> features; // Numeric telemetry vector passed to xinfer
};

} // namespace blackbox