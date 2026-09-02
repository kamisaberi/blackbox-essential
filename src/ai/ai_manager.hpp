#pragma once
#include "blackbox/event.hpp"
#include <xinfer/xinfer.hpp>
#include <memory>
#include <string>
#include "blackbox/export.hpp"  // <--- ADD THIS


namespace blackbox::ai {



    // Fully dynamic model configuration struct
struct BLACKBOX_API ModelConfig {
    xinfer::Target target{xinfer::Target::OpenVINO};
    std::string model_path;
    std::string input_tensor_name{"input"};   // Dynamic input tensor name
    std::string output_tensor_name{"scores"}; // Dynamic output tensor name
    float anomaly_threshold{0.85f};
};

    // Helper: Converts string from blackbox.json into xinfer::Target enum
inline xinfer::Target string_to_xinfer_target(const std::string& target_str) {
    if (target_str == "TensorRT")   return xinfer::Target::TensorRT;
    if (target_str == "OpenVINO")   return xinfer::Target::OpenVINO;
    if (target_str == "RKNN")       return xinfer::Target::RKNN;
    if (target_str == "VitisAI")    return xinfer::Target::VitisAI;
    if (target_str == "QNN")        return xinfer::Target::QNN;
    if (target_str == "CoreML")     return xinfer::Target::CoreML;
    if (target_str == "RyzenAI")    return xinfer::Target::RyzenAI;
    if (target_str == "NeuroPilot") return xinfer::Target::NeuroPilot;
    if (target_str == "Hailo")      return xinfer::Target::Hailo;
    if (target_str == "CVFlow")     return xinfer::Target::CVFlow;
    if (target_str == "ENN")        return xinfer::Target::ENN;
    if (target_str == "EdgeTPU")    return xinfer::Target::EdgeTPU;
    if (target_str == "FPGA_AI")    return xinfer::Target::FPGA_AI;
    if (target_str == "VectorBlox") return xinfer::Target::VectorBlox;
    if (target_str == "SensAI")     return xinfer::Target::SensAI;

    // Default fallback
    return xinfer::Target::OpenVINO;
}



class BLACKBOX_API AIManager {
public:
    explicit AIManager(xinfer::Target target, const std::string& model_path = "");
    explicit AIManager(const std::string& target_str, const std::string& model_path = "");
    ~AIManager() = default;

    // Evaluates security event features using xInfer Essential Engine
    float analyze_event(SecurityEvent& event);

private:
    xinfer::Target target_;

    std::unique_ptr<xinfer::Engine> xinfer_engine_;
    bool is_initialized_{false};
};

} // namespace blackbox::ai