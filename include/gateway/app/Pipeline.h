#pragma once

#include "gateway/business/SQLiteEventStore.h"
#include "gateway/capture/AlsaCapture.h"
#include "gateway/capture/V4L2Capture.h"
#include "gateway/codec/FfmpegEncoder.h"
#include "gateway/concurrency/RingBuffer.h"
#include "gateway/core/Logger.h"
#include "gateway/streaming/EpollRtspServer.h"

#include <atomic>
#include <memory>
#include <thread>

namespace smgw::app {

struct PipelineConfig {
  std::string video_device = "/dev/video0";
  std::string audio_device = "default";
  std::string bind_ip = "0.0.0.0";
  uint16_t rtsp_port = 8554;
  uint32_t width = 1280;
  uint32_t height = 720;
  uint32_t fps = 25;
  uint32_t sample_rate = 48000;
  uint16_t channels = 2;
  std::string log_path = "gateway.log";
  std::string db_path = "gateway.db";
};

class Pipeline {
 public:
  explicit Pipeline(PipelineConfig config);
  ~Pipeline();

  void Start();
  void Stop();

 private:
  void VideoCaptureLoop();
  void AudioCaptureLoop();
  void EncodeLoop();

  PipelineConfig config_;
  core::Logger logger_;
  business::SQLiteEventStore event_store_;
  capture::V4L2Capture video_capture_;
  capture::AlsaCapture audio_capture_;
  codec::FfmpegEncoder encoder_;
  streaming::EpollRtspServer rtsp_server_;

  concurrency::RingBuffer<VideoFrame> video_queue_{128};
  concurrency::RingBuffer<AudioFrame> audio_queue_{128};
  concurrency::RingBuffer<EncodedPacket> packet_queue_{256};

  std::atomic<bool> running_{false};
  std::thread video_thread_;
  std::thread audio_thread_;
  std::thread encode_thread_;
  std::thread stream_thread_;
};

}  // namespace smgw::app
