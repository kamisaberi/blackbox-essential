#include "ai_manager.hpp"
#include <iostream>

namespace blackbox::ai {

AIManager::AIManager(const ModelConfig& config) {
    initialize(config);
}

AIManager::AIManager(xinfer::Target target, 
                     const std::string& model_path, 
                     const std::string& input_tensor_name, 
                     const std::string& output_tensor_name) {
    ModelConfig cfg;
    cfg.target = target;
    cfg.model_path = model_path;
    cfg.input_tensor_name = input_tensor_name;
    cfg.output_tensor_name = output_tensor_name;
    initialize(cfg);
}

bool AIManager::initialize(const ModelConfig& config) {
    config_ = config;
    try {
        // 1. Initialize xInfer Engine for configured hardware target
        xinfer_engine_ = std::make_unique<xinfer::Engine>(config_.target);

        // 2. Load model dynamically (xinfer ModelHub fetches ONNX if missing)
        if (!config_.model_path.empty()) {
            std::cout << "[Blackbox AI] Loading Model via libxinfer.so: " << config_.model_path << std::endl;
            xinfer_engine_->load_model(config_.model_path);
            std::cout << "[Blackbox AI] Model successfully compiled in accelerator RAM!" << std::endl;
        }

        is_initialized_ = true;
        std::cout << "[Blackbox AI] Mapped Input Tensor: [" << config_.input_tensor_name 
                  << "] | Output Tensor: [" << config_.output_tensor_name << "]" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[Blackbox AI Warning] Model load deferred: " << e.what() << std::endl;
        is_initialized_ = false;
        return false;
    }
}

float AIManager::analyze_event(SecurityEvent& event) {
    if (!is_initialized_ || event.features.empty()) {
        return event.anomaly_score; // Heuristic fallback
    }

    try {
        // 1. Get input tensor dynamically using configured tensor name
        xinfer::Tensor& input = xinfer_engine_->get_input_tensor(config_.input_tensor_name);
        input.copy_from_host(event.features.data(), event.features.size() * sizeof(float));

        // 2. Run microsecond AI inference via libxinfer.so
        xinfer_engine_->infer();

        // 3. Get output tensor dynamically using configured tensor name
        xinfer::Tensor& output = xinfer_engine_->get_output_tensor(config_.output_tensor_name);
        return output.data<float>()[0];

    } catch (...) {
        return event.anomaly_score;
    }
}

} // namespace blackbox::ai