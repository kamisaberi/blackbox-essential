#include "gpio_relay.hpp"
#include <iostream>

namespace blackbox::mitigation {

GPIORelay::GPIORelay(int pin_number) : pin_number_(pin_number) {}

void GPIORelay::trigger_alarm(bool enable) {
    std::cout << "[GPIO Hardware] Alarm relay on GPIO pin " << pin_number_
              << (enable ? " -> TRIGGERED SIREN [ON]" : " -> SIREN [OFF]") << std::endl;
}

} // namespace blackbox::mitigation