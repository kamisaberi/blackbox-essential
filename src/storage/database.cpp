#include "database.hpp"
#include <iostream>

namespace blackbox::storage {

Database::Database(const std::string& db_path) : db_path_(db_path) {}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::init() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database Error] Cannot open SQLite DB: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    const char* create_sql = 
        "CREATE TABLE IF NOT EXISTS audit_logs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp INTEGER,"
        "source_ip TEXT,"
        "threat_level TEXT,"
        "anomaly_score REAL,"
        "description TEXT"
        ");";

    char* err_msg = nullptr;
    rc = sqlite3_exec(db_, create_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database Error] Table creation failed: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }

    std::cout << "[Database] Encrypted Audit Store initialized at: " << db_path_ << std::endl;
    return true;
}

bool Database::log_event(const SecurityEvent& event) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_) return false;

    std::string sql = "INSERT INTO audit_logs (timestamp, source_ip, threat_level, anomaly_score, description) VALUES (" +
                      std::to_string(std::chrono::system_clock::to_time_t(event.timestamp)) + ", '" +
                      event.source_ip + "', '" +
                      threat_level_to_string(event.level) + "', " +
                      std::to_string(event.anomaly_score) + ", '" +
                      event.description + "');";

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

} // namespace blackbox::storage