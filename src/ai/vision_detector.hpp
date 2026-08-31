#pragma once

#include "blackbox/export.hpp"
#include "blackbox/event.hpp"
#include <xinfer/xinfer.hpp>
#include <memory>
#include <vector>

namespace blackbox::ai {

class BLACKBOX_API VisionDetector {
public:
    explicit VisionDetector(xinfer::Target target = xinfer::Target::OpenVINO);
    explicit VisionDetector(const std::string& target_str);
    ~VisionDetector() = default;

    float analyze_frame(const std::vector<float>& frame_features);

private:
    std::unique_ptr<xinfer::Engine> xinfer_engine_;
    bool is_ready_{false};
};

} // namespace blackbox::ai