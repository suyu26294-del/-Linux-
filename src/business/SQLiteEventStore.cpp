#include "gateway/business/SQLiteEventStore.h"

#include <sqlite3.h>

#include <stdexcept>

namespace smgw::business {

SQLiteEventStore::SQLiteEventStore(const std::string& db_path) {
  if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
    throw std::runtime_error("Failed to open SQLite DB");
  }
}

SQLiteEventStore::~SQLiteEventStore() {
  if (db_ != nullptr) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

void SQLiteEventStore::InitSchema() {
  constexpr const char* sql =
      "CREATE TABLE IF NOT EXISTS event_log("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "event_type TEXT NOT NULL,"
      "payload TEXT NOT NULL,"
      "ts_epoch_ms INTEGER NOT NULL);"
      "CREATE INDEX IF NOT EXISTS idx_event_log_ts ON event_log(ts_epoch_ms DESC);";

  char* error = nullptr;
  if (sqlite3_exec(db_, sql, nullptr, nullptr, &error) != SQLITE_OK) {
    std::string msg = (error != nullptr) ? error : "unknown sqlite error";
    sqlite3_free(error);
    throw std::runtime_error("InitSchema failed: " + msg);
  }
}

void SQLiteEventStore::InsertEvent(const std::string& event_type, const std::string& payload,
                                   int64_t ts_epoch_ms) {
  constexpr const char* sql =
      "INSERT INTO event_log(event_type, payload, ts_epoch_ms) VALUES (?, ?, ?);";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("InsertEvent prepare failed");
  }

  sqlite3_bind_text(stmt, 1, event_type.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, payload.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 3, ts_epoch_ms);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::runtime_error("InsertEvent execute failed");
  }
  sqlite3_finalize(stmt);
}

std::vector<EventRecord> SQLiteEventStore::QueryRecent(std::size_t limit) const {
  constexpr const char* sql =
      "SELECT id, event_type, payload, ts_epoch_ms FROM event_log "
      "ORDER BY ts_epoch_ms DESC LIMIT ?;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("QueryRecent prepare failed");
  }
  sqlite3_bind_int(stmt, 1, static_cast<int>(limit));

  std::vector<EventRecord> rows;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    EventRecord record;
    record.id = sqlite3_column_int64(stmt, 0);
    record.event_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    record.payload = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    record.ts_epoch_ms = sqlite3_column_int64(stmt, 3);
    rows.push_back(std::move(record));
  }

  sqlite3_finalize(stmt);
  return rows;
}

}  // namespace smgw::business
