#include "gateway/core/Logger.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace smgw::core {

namespace {
const char* ToString(LogLevel level) {
  switch (level) {
    case LogLevel::kDebug:
      return "DEBUG";
    case LogLevel::kInfo:
      return "INFO";
    case LogLevel::kWarn:
      return "WARN";
    case LogLevel::kError:
      return "ERROR";
  }
  return "UNKNOWN";
}
}  // namespace

Logger::Logger(const std::string& file_path, LogLevel level) : min_level_(level), out_(file_path, std::ios::app) {
  if (!out_.is_open()) {
    throw std::runtime_error("Failed to open log file: " + file_path);
  }
  worker_ = std::thread(&Logger::WorkerLoop, this);
}

Logger::~Logger() {
  running_.store(false, std::memory_order_release);
  cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
}

void Logger::Log(LogLevel level, const std::string& message) {
  if (static_cast<int>(level) < static_cast<int>(min_level_)) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(Format(level, message));
  }
  cv_.notify_one();
}

std::string Logger::Format(LogLevel level, const std::string& message) const {
  const auto now = std::chrono::system_clock::now();
  const auto tt = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  gmtime_r(&tt, &tm);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ") << " [" << ToString(level) << "] " << message;
  return oss.str();
}

void Logger::WorkerLoop() {
  while (running_.load(std::memory_order_acquire)) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [&] { return !queue_.empty() || !running_.load(std::memory_order_acquire); });
    while (!queue_.empty()) {
      out_ << queue_.front() << '\n';
      queue_.pop_front();
    }
    out_.flush();
  }
}

}  // namespace smgw::core
