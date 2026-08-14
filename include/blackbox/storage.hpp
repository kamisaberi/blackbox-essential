#pragma once
#include "blackbox/export.hpp"
#include "blackbox/event.hpp"

namespace blackbox::storage {

class BLACKBOX_API IStorageEngine {
public:
    virtual ~IStorageEngine() = default;
    virtual bool log_event(const SecurityEvent& event) = 0;
};

} // namespace blackbox::storage