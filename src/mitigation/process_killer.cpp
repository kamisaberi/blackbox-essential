#include "process_killer.hpp"
#include <iostream>
#include <csignal>

namespace blackbox::mitigation {

bool ProcessKiller::kill_process_by_pid(uint32_t pid) {
    std::cout << "[Process Killer] Terminating malicious PID: " << pid << std::endl;
    return (kill(static_cast<pid_t>(pid), SIGKILL) == 0);
}

} // namespace blackbox::mitigation