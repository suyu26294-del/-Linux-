#include "gateway/codec/FfmpegEncoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <stdexcept>

namespace smgw::codec {

FfmpegEncoder::FfmpegEncoder(uint32_t width, uint32_t height, uint32_t fps, uint32_t sample_rate,
                             uint16_t channels)
    : width_(width), height_(height), fps_(fps), sample_rate_(sample_rate), channels_(channels) {
  InitVideo();
  InitAudio();
}

FfmpegEncoder::~FfmpegEncoder() {
  if (video_ctx_ != nullptr) {
    avcodec_free_context(&video_ctx_);
  }
  if (audio_ctx_ != nullptr) {
    avcodec_free_context(&audio_ctx_);
  }
}

void FfmpegEncoder::InitVideo() {
  const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (codec == nullptr) {
    throw std::runtime_error("Failed to find H.264 encoder");
  }

  video_ctx_ = avcodec_alloc_context3(codec);
  if (video_ctx_ == nullptr) {
    throw std::runtime_error("Failed to allocate video codec context");
  }

  video_ctx_->width = static_cast<int>(width_);
  video_ctx_->height = static_cast<int>(height_);
  video_ctx_->time_base = AVRational{1, static_cast<int>(fps_)};
  video_ctx_->framerate = AVRational{static_cast<int>(fps_), 1};
  video_ctx_->pix_fmt = AV_PIX_FMT_NV12;
  video_ctx_->gop_size = static_cast<int>(fps_ * 2);

  if (avcodec_open2(video_ctx_, codec, nullptr) < 0) {
    throw std::runtime_error("Failed to open H.264 encoder");
  }
}

void FfmpegEncoder::InitAudio() {
  const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
  if (codec == nullptr) {
    throw std::runtime_error("Failed to find AAC encoder");
  }

  audio_ctx_ = avcodec_alloc_context3(codec);
  if (audio_ctx_ == nullptr) {
    throw std::runtime_error("Failed to allocate audio codec context");
  }

  audio_ctx_->sample_rate = static_cast<int>(sample_rate_);
  audio_ctx_->ch_layout.nb_channels = channels_;
  audio_ctx_->sample_fmt = codec->sample_fmts[0];
  audio_ctx_->time_base = AVRational{1, static_cast<int>(sample_rate_)};

  if (avcodec_open2(audio_ctx_, codec, nullptr) < 0) {
    throw std::runtime_error("Failed to open AAC encoder");
  }
}

app::EncodedPacketPtr FfmpegEncoder::EncodeVideo(const app::VideoFramePtr& frame) {
  auto packet = std::make_shared<app::EncodedPacket>();
  packet->is_video = true;
  packet->key_frame = (frame->pts % (fps_ * 2) == 0);
  packet->pts = frame->pts;
  packet->payload = frame->payload;
  return packet;
}

app::EncodedPacketPtr FfmpegEncoder::EncodeAudio(const app::AudioFramePtr& frame) {
  auto packet = std::make_shared<app::EncodedPacket>();
  packet->is_video = false;
  packet->key_frame = false;
  packet->pts = frame->pts;
  packet->payload = frame->payload;
  return packet;
}

}  // namespace smgw::codec
