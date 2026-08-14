#pragma once
#include "blackbox/event.hpp"

namespace blackbox::correlator {

class ThreatScorer {
public:
    ThreatScorer() = default;
    ~ThreatScorer() = default;

    // Evaluates multi-modal scores and frequency to compute final ThreatLevel
    ThreatLevel calculate_threat_level(float raw_anomaly_score, int frequency_burst);
    float compute_composite_risk(float network_score, float vision_score, float log_score);
};

} // namespace blackbox::correlator