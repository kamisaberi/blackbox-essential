# Mapping Model Probabilities to In-Kernel Drop Actions

Once an inference pass completes, continuous output values (such as an autoencoder reconstruction loss or a classification probability) must be translated into discrete, microsecond packet mitigation decisions.

---

## 1. Multi-Tier Escalation Architecture

```text
 Ingress Packet -> Flow Ingestion -> Inference Execution
                                           │
                                           ▼ Evaluation Score (S)
 ┌─────────────────────────────────────────────────────────────────────────────┐
 │ Policy Evaluation Matrix                                                    │
 └─────────────────────────────────────────────────────────────────────────────┘
      │                                   │                               │
      │ S < 0.082                         │ 0.082 <= S < 0.250            │ S >= 0.250
      ▼ (Clean Flow)                      ▼ (Suspicious Deviation)        ▼ (Confirmed Threat)
 ┌─────────────────────────┐         ┌─────────────────────────┐     ┌─────────────────────────┐
 │ Action: XDP_PASS        │         │ Action: Rate Limit /    │     │ Action: XDP_DROP        │
 │ Permitted to Linux stack│         │ Telemetry Flagging      │     │ Insert into in-kernel   │
 │                         │         │ Staged for review       │     │ blocked_ip_map (TTL=60s)│
 └─────────────────────────┘         └─────────────────────────┘     └─────────────────────────┘
```

---

## 2. In-Kernel Action Trigger Implementation

```cpp
#include <blackbox/xdp_manager.hpp>
#include <blackbox/model_config.hpp>

namespace blackbox {

class MitigationArbiter {
public:
    MitigationArbiter(XdpManager& xdp, const MitigationPolicyConfig& policy)
        : xdp_(xdp), policy_(policy) {}

    void evaluate_and_enforce(uint32_t src_ipv4, double score) {
        if (score >= policy_.high_confidence_threshold) {
            // Tier 2 Escalation: Confirmed exploit
            // Block in kernel driver space for extended TTL (e.g., 1 hour)
            xdp_.block_ip(src_ipv4, policy_.escalation_ttl_seconds);
            log_security_event(src_ipv4, score, MitigationAction::KERNEL_DROP_ESCALATED);

        } else if (score >= policy_.anomaly_threshold) {
            // Tier 1 Active Defense: Ephemeral kernel drop
            // Block in kernel driver space for standard TTL (e.g., 60 seconds)
            xdp_.block_ip(src_ipv4, policy_.default_ttl_seconds);
            log_security_event(src_ipv4, score, MitigationAction::KERNEL_DROP);

        } else {
            // Benign flow: No kernel action required; packet passes via XDP_PASS
        }
    }

private:
    XdpManager& xdp_;
    MitigationPolicyConfig policy_;
    void log_security_event(uint32_t ip, double score, MitigationAction action);
};

} // namespace blackbox
```

---

## 3. Dynamic TTL Adaptation

Under active attack conditions, `MitigationArbiter` scales TTL durations dynamically:
* **First Offense:** Blocks for $60\,\text{seconds}$.
* **Repeated Violations:** If an IP triggers an anomaly again within a 10-minute window, the TTL scales exponentially:

$$\text{TTL}_{\text{new}} = \min\left(\text{TTL}_{\text{base}} \times 2^{\text{repeat\_count}},\, 86400\,\text{s}\right)$$

This mechanism shields critical infrastructure while allowing transient network glitches to clear automatically without manual operator intervention.
