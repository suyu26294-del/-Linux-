#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

namespace smgw::core {

enum class LogLevel : uint8_t { kDebug, kInfo, kWarn, kError };

class Logger {
 public:
  Logger(const std::string& file_path, LogLevel level);
  ~Logger();

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  void Log(LogLevel level, const std::string& message);

 private:
  void WorkerLoop();
  std::string Format(LogLevel level, const std::string& message) const;

  std::atomic<bool> running_{true};
  LogLevel min_level_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<std::string> queue_;
  std::ofstream out_;
  std::thread worker_;
};

}  // namespace smgw::core
