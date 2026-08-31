#include "network_detector.hpp"
#include "ai_manager.hpp"
#include <iostream>

namespace blackbox::ai {

NetworkDetector::NetworkDetector(xinfer::Target target) {
    try {
        xinfer_engine_ = std::make_unique<xinfer::Engine>(target);
        is_ready_ = true;
        std::cout << "[Network Detector AI] Connected to xInfer Engine (" 
                  << xinfer::target_to_string(target) << ")." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Network Detector AI Warning] Engine fallback active: " << e.what() << std::endl;
        is_ready_ = false;
    }
}

NetworkDetector::NetworkDetector(const std::string& target_str)
    : NetworkDetector(string_to_xinfer_target(target_str)) {}

float NetworkDetector::analyze_flow(const std::vector<float>& flow_features) {
    if (!is_ready_ || flow_features.empty()) {
        return (flow_features.size() > 2) ? flow_features[2] : 0.1f;
    }

    try {
        xinfer::Tensor& input = xinfer_engine_->get_input_tensor("flow_input");
        input.copy_from_host(flow_features.data(), flow_features.size() * sizeof(float));

        xinfer_engine_->infer();

        xinfer::Tensor& output = xinfer_engine_->get_output_tensor("anomaly_score");
        return output.data<float>()[0];
    } catch (...) {
        return 0.1f;
    }
}

} // namespace blackbox::ai