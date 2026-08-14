#include "rules_engine.hpp"
#include <iostream>

namespace blackbox::correlator {

RulesEngine::RulesEngine(mitigation::EBPFBlocker& blocker) : blocker_(blocker) {}

void RulesEngine::process_event(SecurityEvent& event) {
    // 1. Evaluate Anomaly Score Thresholds
    if (event.anomaly_score >= 0.85f) {
        event.level = ThreatLevel::Critical;
        event.action_taken = ActionType::EBPFBlockIP;
    } else if (event.anomaly_score >= 0.65f) {
        event.level = ThreatLevel::High;
        event.action_taken = ActionType::LogOnly;
    } else {
        event.level = ThreatLevel::Info;
        event.action_taken = ActionType::None;
    }

    // 2. Autonomous Action Execution
    if (event.action_taken == ActionType::EBPFBlockIP && !event.source_ip.empty()) {
        blocker_.block_ip(event.source_ip);
    }

    // 3. Print Threat Event Log
    if (event.level >= ThreatLevel::High) {
        std::cout << "[Blackbox Alert] Threat Level: " << threat_level_to_string(event.level)
                  << " | IP: " << event.source_ip
                  << " | Score: " << event.anomaly_score
                  << " | Action: " << (event.action_taken == ActionType::EBPFBlockIP ? "eBPF IP Blocked" : "Logged")
                  << std::endl;
    }
}

} // namespace blackbox::correlator