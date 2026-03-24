#include "gateway/capture/V4L2Capture.h"

#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>

namespace smgw::capture {

V4L2Capture::V4L2Capture(std::string device, uint32_t width, uint32_t height, uint32_t fps)
    : device_(std::move(device)), width_(width), height_(height), fps_(fps) {}

V4L2Capture::~V4L2Capture() { Stop(); }

void V4L2Capture::Start() {
  if (started_.exchange(true)) {
    return;
  }

  const int fd = ::open(device_.c_str(), O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    core::ThrowSystemError("Open V4L2 device failed");
  }
  fd_.Reset(fd);

  v4l2_format fmt{};
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = width_;
  fmt.fmt.pix.height = height_;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;
  if (ioctl(fd_.Get(), VIDIOC_S_FMT, &fmt) < 0) {
    core::ThrowSystemError("VIDIOC_S_FMT failed");
  }
}

void V4L2Capture::Stop() {
  started_.store(false);
  fd_.Reset();
}

app::VideoFramePtr V4L2Capture::CaptureOne() {
  if (!started_.load()) {
    throw std::runtime_error("V4L2Capture must be started before CaptureOne");
  }

  auto frame = std::make_shared<app::VideoFrame>();
  frame->width = width_;
  frame->height = height_;
  frame->pts = pts_++;
  frame->payload.resize(width_ * height_ * 3 / 2);
  return frame;
}

}  // namespace smgw::capture
