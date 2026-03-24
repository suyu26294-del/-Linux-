#pragma once

#include "gateway/app/MediaTypes.h"

#include <cstdint>

struct AVCodecContext;

namespace smgw::codec {

class FfmpegEncoder {
 public:
  FfmpegEncoder(uint32_t width, uint32_t height, uint32_t fps, uint32_t sample_rate, uint16_t channels);
  ~FfmpegEncoder();

  app::EncodedPacketPtr EncodeVideo(const app::VideoFramePtr& frame);
  app::EncodedPacketPtr EncodeAudio(const app::AudioFramePtr& frame);

 private:
  void InitVideo();
  void InitAudio();

  uint32_t width_;
  uint32_t height_;
  uint32_t fps_;
  uint32_t sample_rate_;
  uint16_t channels_;
  AVCodecContext* video_ctx_ = nullptr;
  AVCodecContext* audio_ctx_ = nullptr;
};

}  // namespace smgw::codec
