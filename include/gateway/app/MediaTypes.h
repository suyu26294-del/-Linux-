#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace smgw::app {

struct VideoFrame {
  uint32_t width = 0;
  uint32_t height = 0;
  int64_t pts = 0;
  std::vector<uint8_t> payload;
};

struct AudioFrame {
  uint32_t sample_rate = 0;
  uint16_t channels = 0;
  int64_t pts = 0;
  std::vector<uint8_t> payload;
};

struct EncodedPacket {
  bool is_video = true;
  bool key_frame = false;
  int64_t pts = 0;
  std::vector<uint8_t> payload;
};

using VideoFramePtr = std::shared_ptr<VideoFrame>;
using AudioFramePtr = std::shared_ptr<AudioFrame>;
using EncodedPacketPtr = std::shared_ptr<EncodedPacket>;

}  // namespace smgw::app
