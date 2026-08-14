#include "ring_buffer.hpp"

namespace blackbox::storage {

EventRingBuffer::EventRingBuffer(size_t capacity) : capacity_(capacity) {
    buffer_.resize(capacity_);
}

bool EventRingBuffer::push(const SecurityEvent& event) {
    size_t current_tail = tail_.load(std::memory_order_relaxed);
    size_t next_tail = (current_tail + 1) % capacity_;

    if (next_tail == head_.load(std::memory_order_acquire)) {
        return false; // Queue full
    }

    buffer_[current_tail] = event;
    tail_.store(next_tail, std::memory_order_release);
    return true;
}

std::optional<SecurityEvent> EventRingBuffer::pop() {
    size_t current_head = head_.load(std::memory_order_relaxed);

    if (current_head == tail_.load(std::memory_order_acquire)) {
        return std::nullopt; // Queue empty
    }

    SecurityEvent event = buffer_[current_head];
    head_.store((current_head + 1) % capacity_, std::memory_order_release);
    return event;
}

size_t EventRingBuffer::size() const {
    size_t h = head_.load(std::memory_order_relaxed);
    size_t t = tail_.load(std::memory_order_relaxed);
    return (t >= h) ? (t - h) : (capacity_ - h + t);
}

} // namespace blackbox::storage