#pragma once
#include "blackbox/export.hpp"
#include "blackbox/event.hpp"
#include <vector>

namespace blackbox::correlator {

class BLACKBOX_API CorrelatorEngine {
public:
    CorrelatorEngine() = default;
    ~CorrelatorEngine() = default;

    ThreatLevel evaluate_threat(const SecurityEvent& event);
};

} // namespace blackbox::correlator