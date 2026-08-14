#include <iostream>
#include <cassert>
#include "mitigation/ebpf_blocker.hpp"
#include "mitigation/process_killer.hpp"
#include "mitigation/gpio_relay.hpp"

int main() {
    std::cout << "[Test Mitigation] Starting autonomous enforcement verification..." << std::endl;

    // 1. Test eBPF IP Blocker
    blackbox::mitigation::EBPFBlocker blocker;
    std::string test_ip = "192.168.1.250";

    bool blocked = blocker.block_ip(test_ip);
    assert(blocked == true);

    // Re-blocking same IP should return true (already blocked)
    bool reblocked = blocker.block_ip(test_ip);
    assert(reblocked == true);

    bool unblocked = blocker.unblock_ip(test_ip);
    assert(unblocked == true);

    // 2. Test GPIO Relay Trigger
    blackbox::mitigation::GPIORelay relay(18);
    relay.trigger_alarm(true);  // Trigger siren ON
    relay.trigger_alarm(false); // Trigger siren OFF

    // 3. Test Process Killer (Attempt killing invalid PID 999999 safely)
    bool killed = blackbox::mitigation::ProcessKiller::kill_process_by_pid(999999);
    assert(killed == false); // Expected false because PID doesn't exist

    std::cout << "[PASS] Autonomous mitigation engines verified successfully." << std::endl;
    return 0;
}