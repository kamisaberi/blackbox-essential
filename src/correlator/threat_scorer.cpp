#include "threat_scorer.hpp"
#include <algorithm>

namespace blackbox::correlator {

float ThreatScorer::compute_composite_risk(float network_score, float vision_score, float log_score) {
    // Weighted risk aggregation formula
    float weighted_score = (network_score * 0.45f) + (vision_score * 0.35f) + (log_score * 0.20f);
    return std::min(1.0f, weighted_score);
}

ThreatLevel ThreatScorer::calculate_threat_level(float raw_anomaly_score, int frequency_burst) {
    float adjusted_score = raw_anomaly_score + (frequency_burst * 0.02f);

    if (adjusted_score >= 0.85f) return ThreatLevel::Critical;
    if (adjusted_score >= 0.65f) return ThreatLevel::High;
    if (adjusted_score >= 0.45f) return ThreatLevel::Medium;
    if (adjusted_score >= 0.25f) return ThreatLevel::Low;
    
    return ThreatLevel::Info;
}

} // namespace blackbox::correlator