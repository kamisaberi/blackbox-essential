#pragma once
#include "blackbox/event.hpp"
#include <xinfer/xinfer.hpp>
#include <memory>
#include <string>
#include "blackbox/export.hpp"  // <--- ADD THIS


namespace blackbox::ai {

class AIManager {
public:
    AIManager(xinfer::Target target, const std::string& model_path);
    ~AIManager() = default;

    // Evaluates security event features using xInfer Essential Engine
    float analyze_event(SecurityEvent& event);

private:
    std::unique_ptr<xinfer::Engine> xinfer_engine_;
    bool is_initialized_{false};
};

} // namespace blackbox::ai