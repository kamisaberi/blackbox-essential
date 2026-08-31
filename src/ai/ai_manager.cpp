#include "ai_manager.hpp"
#include <iostream>

namespace blackbox::ai {

AIManager::AIManager(xinfer::Target target, const std::string& model_path)
    : target_(target) {
    try {
        xinfer_engine_ = std::make_unique<xinfer::Engine>(target_);
        if (!model_path.empty()) {
            xinfer_engine_->load_model(model_path);
        }
        is_initialized_ = true;
        std::cout << "[Blackbox AI] Embedded xInfer Engine initialized on platform: " 
                  << xinfer::target_to_string(target_) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Blackbox AI Warning] xInfer Engine initialization deferred for platform (" 
                  << xinfer::target_to_string(target_) << "): " << e.what() << std::endl;
        is_initialized_ = false;
    }
}

AIManager::AIManager(const std::string& target_str, const std::string& model_path)
    : AIManager(string_to_xinfer_target(target_str), model_path) {}

float AIManager::analyze_event(SecurityEvent& event) {
    if (!is_initialized_ || event.features.empty()) {
        return event.anomaly_score; // Fallback heuristic
    }

    try {
        xinfer::Tensor& input = xinfer_engine_->get_input_tensor("features");
        input.copy_from_host(event.features.data(), event.features.size() * sizeof(float));

        // Execute microsecond AI inference via libxinfer.so
        xinfer_engine_->infer();

        xinfer::Tensor& output = xinfer_engine_->get_output_tensor("score");
        return output.data<float>()[0];

    } catch (...) {
        return event.anomaly_score;
    }
}

} // namespace blackbox::ai