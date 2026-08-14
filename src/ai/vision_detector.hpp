#pragma once
#include "blackbox/event.hpp"
#include <xinfer/xinfer.hpp>
#include <memory>
#include <vector>

namespace blackbox::ai {

class VisionDetector {
public:
    VisionDetector();
    ~VisionDetector() = default;

    // Evaluates camera frame bounding box features using xInfer Essential
    float analyze_frame(const std::vector<float>& frame_features);

private:
    std::unique_ptr<xinfer::Engine> xinfer_engine_;
    bool is_ready_{false};
};

} // namespace blackbox::ai