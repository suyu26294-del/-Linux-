#pragma once

#include <unistd.h>

#include <stdexcept>

namespace smgw::core {

class Fd {
 public:
  Fd() = default;
  explicit Fd(int fd) : fd_(fd) {}

  Fd(const Fd&) = delete;
  Fd& operator=(const Fd&) = delete;

  Fd(Fd&& other) noexcept;
  Fd& operator=(Fd&& other) noexcept;

  ~Fd();

  [[nodiscard]] int Get() const { return fd_; }
  [[nodiscard]] bool Valid() const { return fd_ >= 0; }

  int Release();
  void Reset(int fd = -1);

 private:
  int fd_ = -1;
};

void ThrowSystemError(const char* context);

}  // namespace smgw::core
