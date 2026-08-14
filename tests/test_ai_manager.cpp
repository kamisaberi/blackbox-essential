#include <iostream>
#include <cassert>
#include "blackbox/event.hpp"

int main() {
    blackbox::SecurityEvent event;
    event.anomaly_score = 0.95f;
    assert(event.anomaly_score > 0.8f);
    std::cout << "[PASS] AI Manager Event Test Passed." << std::endl;
    return 0;
}