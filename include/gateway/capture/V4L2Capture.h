#pragma once

#include "gateway/app/MediaTypes.h"
#include "gateway/core/Fd.h"

#include <atomic>
#include <string>

namespace smgw::capture {

class V4L2Capture {
 public:
  V4L2Capture(std::string device, uint32_t width, uint32_t height, uint32_t fps);
  ~V4L2Capture();

  V4L2Capture(const V4L2Capture&) = delete;
  V4L2Capture& operator=(const V4L2Capture&) = delete;

  void Start();
  void Stop();
  app::VideoFramePtr CaptureOne();

 private:
  std::string device_;
  uint32_t width_;
  uint32_t height_;
  uint32_t fps_;
  std::atomic<bool> started_{false};
  core::Fd fd_;
  int64_t pts_ = 0;
};

}  // namespace smgw::capture
