#include <iostream>
#include <cassert>
#include "correlator/rules_engine.hpp"
#include "correlator/threat_scorer.hpp"
#include "mitigation/ebpf_blocker.hpp"

int main() {
    std::cout << "[Test Correlator] Verifying threat correlation & scoring math..." << std::endl;

    blackbox::mitigation::EBPFBlocker blocker;
    blackbox::correlator::RulesEngine rules_engine(blocker);
    blackbox::correlator::ThreatScorer threat_scorer;

    // 1. Test Threat Scoring Formula
    float composite_score = threat_scorer.compute_composite_risk(0.9f, 0.8f, 0.5f);
    assert(composite_score > 0.7f && composite_score <= 1.0f);

    blackbox::ThreatLevel level_high = threat_scorer.calculate_threat_level(0.70f, 0);
    assert(level_high == blackbox::ThreatLevel::High);

    blackbox::ThreatLevel level_critical = threat_scorer.calculate_threat_level(0.85f, 2);
    assert(level_critical == blackbox::ThreatLevel::Critical);

    // 2. Test Rules Engine Execution for Critical Threat
    blackbox::SecurityEvent critical_event;
    critical_event.event_id = 101;
    critical_event.source_ip = "10.0.0.99";
    critical_event.anomaly_score = 0.95f; // Critical threshold >= 0.85f

    rules_engine.process_event(critical_event);

    assert(critical_event.level == blackbox::ThreatLevel::Critical);
    assert(critical_event.action_taken == blackbox::ActionType::EBPFBlockIP);

    // 3. Test Rules Engine Execution for Info Event
    blackbox::SecurityEvent info_event;
    info_event.event_id = 102;
    info_event.source_ip = "10.0.0.5";
    info_event.anomaly_score = 0.15f; // Low score

    rules_engine.process_event(info_event);

    assert(info_event.level == blackbox::ThreatLevel::Info);
    assert(info_event.action_taken == blackbox::ActionType::None);

    std::cout << "[PASS] Threat correlation rules and scoring engine verified." << std::endl;
    return 0;
}