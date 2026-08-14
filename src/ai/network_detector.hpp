#pragma once
#include "blackbox/event.hpp"
#include <xinfer/xinfer.hpp>
#include <memory>
#include <vector>

namespace blackbox::ai {

class NetworkDetector {
public:
    NetworkDetector();
    ~NetworkDetector() = default;

    // Evaluates network flow telemetry vectors using xInfer Essential
    float analyze_flow(const std::vector<float>& flow_features);

private:
    std::unique_ptr<xinfer::Engine> xinfer_engine_;
    bool is_ready_{false};
};

} // namespace blackbox::ai