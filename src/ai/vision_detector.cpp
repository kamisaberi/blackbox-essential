#include "vision_detector.hpp"
#include "ai_manager.hpp"
#include <iostream>

namespace blackbox::ai {

VisionDetector::VisionDetector(xinfer::Target target) {
    try {
        xinfer_engine_ = std::make_unique<xinfer::Engine>(target);
        is_ready_ = true;
        std::cout << "[Vision Detector AI] Connected to xInfer Engine (" 
                  << xinfer::target_to_string(target) << ")." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Vision Detector AI Warning] Engine fallback active: " << e.what() << std::endl;
        is_ready_ = false;
    }
}

VisionDetector::VisionDetector(const std::string& target_str)
    : VisionDetector(string_to_xinfer_target(target_str)) {}

float VisionDetector::analyze_frame(const std::vector<float>& frame_features) {
    if (!is_ready_ || frame_features.empty()) {
        return (frame_features.size() > 1) ? frame_features[1] : 0.05f;
    }

    try {
        xinfer::Tensor& input = xinfer_engine_->get_input_tensor("images");
        input.copy_from_host(frame_features.data(), frame_features.size() * sizeof(float));

        xinfer_engine_->infer();

        xinfer::Tensor& output = xinfer_engine_->get_output_tensor("output0");
        return output.data<float>()[0];
    } catch (...) {
        return 0.05f;
    }
}

} // namespace blackbox::ai