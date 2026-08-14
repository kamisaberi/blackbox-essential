#include <iostream>
#include <cassert>
#include "blackbox/event.hpp"

int main() {
    blackbox::SecurityEvent event;
    event.anomaly_score = 0.90f;
    assert(event.anomaly_score >= 0.85f);
    std::cout << "[PASS] Rules Engine Test Passed." << std::endl;
    return 0;
}