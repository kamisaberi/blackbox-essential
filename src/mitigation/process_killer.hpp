#pragma once
#include <cstdint>

namespace blackbox::mitigation {

class ProcessKiller {
public:
    static bool kill_process_by_pid(uint32_t pid);
};

} // namespace blackbox::mitigation