#pragma once

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace smgw::concurrency {

template <typename T>
class RingBuffer {
 public:
  explicit RingBuffer(std::size_t capacity) : capacity_(capacity), data_(capacity) {}

  void Push(std::shared_ptr<T> item) {
    std::unique_lock<std::mutex> lock(mutex_);
    producer_cv_.wait(lock, [&] { return size_ < capacity_ || stopped_; });
    if (stopped_) {
      return;
    }
    data_[tail_] = std::move(item);
    tail_ = (tail_ + 1) % capacity_;
    ++size_;
    consumer_cv_.notify_one();
  }

  std::optional<std::shared_ptr<T>> Pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    consumer_cv_.wait(lock, [&] { return size_ > 0 || stopped_; });
    if (size_ == 0) {
      return std::nullopt;
    }

    auto item = std::move(data_[head_]);
    data_[head_].reset();
    head_ = (head_ + 1) % capacity_;
    --size_;
    producer_cv_.notify_one();
    return item;
  }

  void Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    stopped_ = true;
    producer_cv_.notify_all();
    consumer_cv_.notify_all();
  }

 private:
  std::size_t capacity_;
  std::vector<std::shared_ptr<T>> data_;
  std::size_t head_ = 0;
  std::size_t tail_ = 0;
  std::size_t size_ = 0;
  bool stopped_ = false;
  std::mutex mutex_;
  std::condition_variable producer_cv_;
  std::condition_variable consumer_cv_;
};

}  // namespace smgw::concurrency
