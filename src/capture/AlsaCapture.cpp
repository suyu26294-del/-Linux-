#include "gateway/capture/AlsaCapture.h"

#include <stdexcept>

namespace smgw::capture {

AlsaCapture::AlsaCapture(std::string device, uint32_t sample_rate, uint16_t channels)
    : device_(std::move(device)), sample_rate_(sample_rate), channels_(channels) {}

AlsaCapture::~AlsaCapture() { Stop(); }

void AlsaCapture::Start() {
  if (started_.exchange(true)) {
    return;
  }

  if (snd_pcm_open(&pcm_handle_, device_.c_str(), SND_PCM_STREAM_CAPTURE, 0) < 0) {
    throw std::runtime_error("snd_pcm_open failed");
  }

  if (snd_pcm_set_params(pcm_handle_, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                         channels_, sample_rate_, 1, 20000) < 0) {
    throw std::runtime_error("snd_pcm_set_params failed");
  }
}

void AlsaCapture::Stop() {
  started_.store(false);
  if (pcm_handle_ != nullptr) {
    snd_pcm_close(pcm_handle_);
    pcm_handle_ = nullptr;
  }
}

app::AudioFramePtr AlsaCapture::CaptureOne() {
  if (!started_.load()) {
    throw std::runtime_error("AlsaCapture must be started before CaptureOne");
  }

  constexpr uint32_t kSamplesPerFrame = 1024;
  auto frame = std::make_shared<app::AudioFrame>();
  frame->sample_rate = sample_rate_;
  frame->channels = channels_;
  frame->pts = pts_;
  pts_ += kSamplesPerFrame;

  frame->payload.resize(kSamplesPerFrame * channels_ * sizeof(int16_t));
  const auto read_samples = snd_pcm_readi(pcm_handle_, frame->payload.data(), kSamplesPerFrame);
  if (read_samples < 0) {
    snd_pcm_prepare(pcm_handle_);
    throw std::runtime_error("snd_pcm_readi failed");
  }

  return frame;
}

}  // namespace smgw::capture
