#include "ai_manager.hpp"
#include <iostream>

namespace blackbox::ai {

AIManager::AIManager(xinfer::Target target, const std::string& model_path) {
    try {
        xinfer_engine_ = std::make_unique<xinfer::Engine>(target);
        if (!model_path.empty()) {
            // xinfer_engine_->load_model(model_path);
        }
        is_initialized_ = true;
        std::cout << "[Blackbox AI] Embedded xInfer Engine initialized on target: " 
                  << xinfer::target_to_string(target) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Blackbox AI Warning] xInfer Engine initialization deferred: " << e.what() << std::endl;
    }
}

float AIManager::analyze_event(SecurityEvent& event) {
    if (!is_initialized_ || event.features.empty()) {
        return event.anomaly_score; 
    }

    try {
        xinfer::Tensor& input = xinfer_engine_->get_input_tensor("features");
        input.copy_from_host(event.features.data(), event.features.size() * sizeof(float));

        xinfer_engine_->infer();

        xinfer::Tensor& output = xinfer_engine_->get_output_tensor("score");
        return output.data<float>()[0];

    } catch (...) {
        return event.anomaly_score;
    }
}

} // namespace blackbox::ai