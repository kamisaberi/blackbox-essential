#pragma once
#include <string>
#include <cstdint>

namespace blackbox {

enum class EventType {
    NetworkPacket,
    SystemLog,
    VisionCamera
};

enum class ThreatLevel {
    Info,
    Low,
    Medium,
    High,
    Critical
};

enum class ActionType {
    None,
    LogOnly,
    EBPFBlockIP,
    KillProcess,
    TriggerAlarm
};

inline std::string threat_level_to_string(ThreatLevel level) {
    switch (level) {
        case ThreatLevel::Info:     return "INFO";
        case ThreatLevel::Low:      return "LOW";
        case ThreatLevel::Medium:   return "MEDIUM";
        case ThreatLevel::High:     return "HIGH";
        case ThreatLevel::Critical: return "CRITICAL";
        default:                    return "UNKNOWN";
    }
}

} // namespace blackbox