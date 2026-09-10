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

inline std::string action_type_to_string(ActionType action) {
    switch (action) {
        case ActionType::None:         return "None";
        case ActionType::LogOnly:      return "Log Only";
        case ActionType::EBPFBlockIP:  return "eBPF Kernel Drop";
        case ActionType::KillProcess:  return "Process Terminated";
        case ActionType::TriggerAlarm: return "Hardware Alarm Triggered";
        default:                       return "Unknown";
    }
}


} // namespace blackbox