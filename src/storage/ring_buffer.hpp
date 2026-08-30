#pragma once
#include "blackbox/event.hpp"
#include <vector>
#include <atomic>
#include <optional>
#include <cstddef>
#include "blackbox/export.hpp"  // <--- ADD THIS

namespace blackbox::storage {

class BLACKBOX_API  EventRingBuffer {
public:
    explicit EventRingBuffer(size_t capacity = 1024);
    ~EventRingBuffer() = default;

    bool push(const SecurityEvent& event);
    std::optional<SecurityEvent> pop();
    size_t capacity() const { return capacity_; }
    size_t size() const;

private:
    size_t capacity_;
    std::vector<SecurityEvent> buffer_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};

} // namespace blackbox::storage