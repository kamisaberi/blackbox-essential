#include <iostream>
#include <cassert>
#include "ai/ai_manager.hpp"
#include "ai/network_detector.hpp"
#include "ai/vision_detector.hpp"

int main() {
    std::cout << "[Test AI Manager] Verifying xInfer Engine integration..." << std::endl;

    // 1. Initialize AI Manager
    blackbox::ai::AIManager ai_manager(xinfer::Target::TensorRT, "models/threat_detector.engine");

    // 2. Create Test Event with Feature Vector
    blackbox::SecurityEvent test_event;
    test_event.event_id = 5001;
    test_event.type = blackbox::EventType::NetworkPacket;
    test_event.features = {0.15f, 0.88f, 0.92f, 0.45f};
    test_event.anomaly_score = 0.50f;

    // 3. Execute Analysis via AI Manager
    float computed_score = ai_manager.analyze_event(test_event);
    assert(computed_score >= 0.0f && computed_score <= 1.0f);

    // 4. Test Network Detector
    blackbox::ai::NetworkDetector net_detector;
    float net_score = net_detector.analyze_flow(test_event.features);
    assert(net_score >= 0.0f && net_score <= 1.0f);

    // 5. Test Vision Detector
    blackbox::ai::VisionDetector vision_detector;
    float vision_score = vision_detector.analyze_frame({0.9f, 0.8f, 0.7f});
    assert(vision_score >= 0.0f && vision_score <= 1.0f);

    std::cout << "[PASS] AI Engine manager and sub-detectors verified." << std::endl;
    return 0;
}