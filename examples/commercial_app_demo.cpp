#include <iostream>
#include <thread>
#include <chrono>
#include <blackbox/blackbox.hpp>

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " Commercial Security Product (Powered by libblackbox.so)  " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // 1. Initialize Blackbox Shared Library Engine
    blackbox::BlackboxEngine security_engine("configs/blackbox.json");

    // 2. Start Autonomous Defense Services
    security_engine.start();

    // 3. Submit Security Telemetry Events from Product
    blackbox::SecurityEvent event;
    event.event_id = 1001;
    event.type = blackbox::EventType::NetworkPacket;
    event.source_ip = "192.168.1.250";
    event.features = {0.1f, 0.9f, 0.95f, 0.3f};

    std::cout << "Submitting network telemetry event to libblackbox.so..." << std::endl;
    security_engine.submit_event(event);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 4. Graceful Shutdown
    security_engine.stop();
    std::cout << "Commercial Application execution finished." << std::endl;
    return 0;
}