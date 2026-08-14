#include "network_detector.hpp"
#include <iostream>

namespace blackbox::ai {

NetworkDetector::NetworkDetector() {
    try {
        xinfer_engine_ = std::make_unique<xinfer::Engine>(xinfer::Target::TensorRT);
        is_ready_ = true;
        std::cout << "[Network Detector AI] Connected to xInfer Essential Engine." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Network Detector AI] Fallback mode active: " << e.what() << std::endl;
        is_ready_ = false;
    }
}

float NetworkDetector::analyze_flow(const std::vector<float>& flow_features) {
    if (!is_ready_ || flow_features.empty()) {
        // Fallback heuristic scoring if xInfer engine is not compiled with TRT on current host
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