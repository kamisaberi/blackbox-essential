#pragma once
#include "blackbox/export.hpp"
#include "blackbox/types.hpp"
#include <string>

namespace blackbox::mitigation {

class BLACKBOX_API MitigationEngine {
public:
    virtual ~MitigationEngine() = default;
    virtual bool execute_action(ActionType action, const std::string& target) = 0;
};

} // namespace blackbox::mitigation