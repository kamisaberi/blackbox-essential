#pragma once

namespace blackbox::mitigation {

class GPIORelay {
public:
    explicit GPIORelay(int pin_number);
    void trigger_alarm(bool enable);

private:
    int pin_number_;
};

} // namespace blackbox::mitigation