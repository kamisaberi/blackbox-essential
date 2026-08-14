#include "vision_detector.hpp"
#include <iostream>

namespace blackbox::ai {

VisionDetector::VisionDetector() {
    try {
        xinfer_engine_ = std::make_unique<xinfer::Engine>(xinfer::Target::TensorRT);
        is_ready_ = true;
        std::cout << "[Vision Detector AI] Connected to xInfer Essential Engine." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Vision Detector AI] Fallback mode active: " << e.what() << std::endl;
        is_ready_ = false;
    }
}

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