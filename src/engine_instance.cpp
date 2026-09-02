#include "blackbox/engine_instance.hpp"
#include "blackbox/event.hpp"
#include "ai/ai_manager.hpp"
#include "correlator/rules_engine.hpp"
#include "mitigation/ebpf_blocker.hpp"
#include "storage/ring_buffer.hpp"
#include "storage/database.hpp"

#include <iostream>
#include <thread>
#include <atomic>

namespace blackbox {

class BlackboxEngine::Impl {
public:
    explicit Impl(const std::string& config_path)
        : db_("blackbox_audit.db"),
          ebpf_blocker_(),
          rules_engine_(ebpf_blocker_),
          ai_manager_(xinfer::Target::OpenVINO, "version-RFB-320.onnx", "input", "scores") {
        std::cout << "[libblackbox.so] Initialized Blackbox Engine Instance (" << config_path << ")" << std::endl;
    }

    bool start() {
        if (running_) return true;
        running_ = true;

        worker_thread_ = std::thread([this]() {
            while (running_) {
                auto event_opt = ring_buffer_.pop();
                if (event_opt.has_value()) {
                    auto event = event_opt.value();
                    event.anomaly_score = ai_manager_.analyze_event(event);
                    rules_engine_.process_event(event);
                    db_.log_event(event);
                } else {
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }
        });

        std::cout << "[libblackbox.so] Engine processing loop started." << std::endl;
        return true;
    }

    void stop() {
        if (!running_) return;
        running_ = false;
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        std::cout << "[libblackbox.so] Engine processing loop stopped." << std::endl;
    }

    void submit_event(const SecurityEvent& event) {
        ring_buffer_.push(event);
    }

    bool block_ip(const std::string& ip) {
        return ebpf_blocker_.block_ip(ip);
    }

    bool unblock_ip(const std::string& ip) {
        return ebpf_blocker_.unblock_ip(ip);
    }

    bool is_running() const { return running_; }

private:
    std::atomic<bool> running_{false};
    std::thread worker_thread_;

    storage::EventRingBuffer ring_buffer_{2048};
    storage::Database db_;
    mitigation::EBPFBlocker ebpf_blocker_;
    correlator::RulesEngine rules_engine_;
    ai::AIManager ai_manager_;
};

BlackboxEngine::BlackboxEngine(const std::string& config_file_path)
    : impl_(std::make_unique<Impl>(config_file_path)) {}

BlackboxEngine::~BlackboxEngine() = default;

bool BlackboxEngine::start() { return impl_->start(); }
void BlackboxEngine::stop() { impl_->stop(); }
void BlackboxEngine::submit_event(const SecurityEvent& event) { impl_->submit_event(event); }
bool BlackboxEngine::block_ip_address(const std::string& ip) { return impl_->block_ip(ip); }
bool BlackboxEngine::unblock_ip_address(const std::string& ip) { return impl_->unblock_ip(ip); }
bool BlackboxEngine::is_running() const { return impl_->is_running(); }

} // namespace blackbox