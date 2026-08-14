#pragma once
#include "blackbox/event.hpp"
#include "mitigation/ebpf_blocker.hpp"

namespace blackbox::correlator {

class RulesEngine {
public:
    explicit RulesEngine(mitigation::EBPFBlocker& blocker);
    ~RulesEngine() = default;

    void process_event(SecurityEvent& event);

private:
    mitigation::EBPFBlocker& blocker_;
};

} // namespace blackbox::correlator