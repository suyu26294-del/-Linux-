#include "gateway/core/Fd.h"

#include <cerrno>
#include <cstring>

namespace smgw::core {

Fd::Fd(Fd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }

Fd& Fd::operator=(Fd&& other) noexcept {
  if (this != &other) {
    Reset();
    fd_ = other.fd_;
    other.fd_ = -1;
  }
  return *this;
}

Fd::~Fd() { Reset(); }

int Fd::Release() {
  int tmp = fd_;
  fd_ = -1;
  return tmp;
}

void Fd::Reset(int fd) {
  if (fd_ >= 0) {
    ::close(fd_);
  }
  fd_ = fd;
}

void ThrowSystemError(const char* context) {
  throw std::runtime_error(std::string(context) + ": " + std::strerror(errno));
}

}  // namespace smgw::core
