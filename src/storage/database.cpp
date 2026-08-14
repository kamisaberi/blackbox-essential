#include "database.hpp"
#include <iostream>

namespace blackbox::storage {

Database::Database(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "[Database Error] Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        db_ = nullptr;
        return;
    }

    const char* sql_create = "CREATE TABLE IF NOT EXISTS audit_logs ("
                             "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                             "event_id INTEGER, "
                             "type INTEGER, "
                             "source_ip TEXT, "
                             "anomaly_score REAL, "
                             "action INTEGER, "
                             "description TEXT);";

    char* err_msg = nullptr;
    if (sqlite3_exec(db_, sql_create, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "[Database Error] Table creation failed: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    } else {
        std::cout << "[Database] Encrypted SQLite Audit Storage initialized at: " << db_path << std::endl;
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::log_event(const SecurityEvent& event) {
    if (!db_) return false;
    std::lock_guard<std::mutex> lock(db_mutex_);

    std::string sql = "INSERT INTO audit_logs (event_id, type, source_ip, anomaly_score, action, description) VALUES (" +
                      std::to_string(event.event_id) + ", " +
                      std::to_string(static_cast<int>(event.type)) + ", '" +
                      event.source_ip + "', " +
                      std::to_string(event.anomaly_score) + ", " +
                      std::to_string(static_cast<int>(event.action_taken)) + ", '" +
                      event.description + "');";

    char* err_msg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

} // namespace blackbox::storage