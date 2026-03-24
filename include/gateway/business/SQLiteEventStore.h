#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct sqlite3;

namespace smgw::business {

struct EventRecord {
  int64_t id = 0;
  std::string event_type;
  std::string payload;
  int64_t ts_epoch_ms = 0;
};

class SQLiteEventStore {
 public:
  explicit SQLiteEventStore(const std::string& db_path);
  ~SQLiteEventStore();

  void InitSchema();
  void InsertEvent(const std::string& event_type, const std::string& payload, int64_t ts_epoch_ms);
  std::vector<EventRecord> QueryRecent(std::size_t limit) const;

 private:
  sqlite3* db_ = nullptr;
};

}  // namespace smgw::business
