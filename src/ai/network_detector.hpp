#pragma once

#include "blackbox/export.hpp"
#include "blackbox/event.hpp"
#include <xinfer/xinfer.hpp>
#include <memory>
#include <vector>

namespace blackbox::ai {

class BLACKBOX_API NetworkDetector {
public:
    explicit NetworkDetector(xinfer::Target target = xinfer::Target::OpenVINO);
    explicit NetworkDetector(const std::string& target_str);
    ~NetworkDetector() = default;

    float analyze_flow(const std::vector<float>& flow_features);

private:
    std::unique_ptr<xinfer::Engine> xinfer_engine_;
    bool is_ready_{false};
};

} // namespace blackbox::ai