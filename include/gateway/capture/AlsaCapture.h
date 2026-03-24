#pragma once

#include "gateway/app/MediaTypes.h"

#include <atomic>
#include <string>

#include <alsa/asoundlib.h>

namespace smgw::capture {

class AlsaCapture {
 public:
  AlsaCapture(std::string device, uint32_t sample_rate, uint16_t channels);
  ~AlsaCapture();

  void Start();
  void Stop();
  app::AudioFramePtr CaptureOne();

 private:
  std::string device_;
  uint32_t sample_rate_;
  uint16_t channels_;
  snd_pcm_t* pcm_handle_ = nullptr;
  std::atomic<bool> started_{false};
  int64_t pts_ = 0;
};

}  // namespace smgw::capture
