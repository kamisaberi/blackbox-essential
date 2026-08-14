#pragma once
#include "blackbox/event.hpp"
#include <sqlite3.h>
#include <string>
#include <mutex>

namespace blackbox::storage {

class Database {
public:
    explicit Database(const std::string& db_path);
    ~Database();

    bool log_event(const SecurityEvent& event);

private:
    sqlite3* db_{nullptr};
    std::mutex db_mutex_;
};

} // namespace blackbox::storage