#include "gateway/app/Pipeline.h"

#include <chrono>

namespace smgw::app {

Pipeline::Pipeline(PipelineConfig config)
    : config_(std::move(config)),
      logger_(config_.log_path, core::LogLevel::kInfo),
      event_store_(config_.db_path),
      video_capture_(config_.video_device, config_.width, config_.height, config_.fps),
      audio_capture_(config_.audio_device, config_.sample_rate, config_.channels),
      encoder_(config_.width, config_.height, config_.fps, config_.sample_rate, config_.channels),
      rtsp_server_(config_.bind_ip, config_.rtsp_port) {}

Pipeline::~Pipeline() { Stop(); }

void Pipeline::Start() {
  if (running_.exchange(true)) {
    return;
  }

  event_store_.InitSchema();
  event_store_.InsertEvent("system", "gateway started", std::chrono::duration_cast<std::chrono::milliseconds>(
                                                         std::chrono::system_clock::now().time_since_epoch())
                                                         .count());

  video_capture_.Start();
  audio_capture_.Start();
  rtsp_server_.Start();

  video_thread_ = std::thread(&Pipeline::VideoCaptureLoop, this);
  audio_thread_ = std::thread(&Pipeline::AudioCaptureLoop, this);
  encode_thread_ = std::thread(&Pipeline::EncodeLoop, this);
  stream_thread_ = std::thread([this] {
    while (running_.load()) {
      auto packet = packet_queue_.Pop();
      if (!packet.has_value()) {
        break;
      }
      rtsp_server_.Broadcast(*packet);
    }
  });
}

void Pipeline::Stop() {
  if (!running_.exchange(false)) {
    return;
  }

  video_queue_.Stop();
  audio_queue_.Stop();
  packet_queue_.Stop();

  if (video_thread_.joinable()) {
    video_thread_.join();
  }
  if (audio_thread_.joinable()) {
    audio_thread_.join();
  }
  if (encode_thread_.joinable()) {
    encode_thread_.join();
  }
  if (stream_thread_.joinable()) {
    stream_thread_.join();
  }

  rtsp_server_.Stop();
  video_capture_.Stop();
  audio_capture_.Stop();

  event_store_.InsertEvent("system", "gateway stopped", std::chrono::duration_cast<std::chrono::milliseconds>(
                                                         std::chrono::system_clock::now().time_since_epoch())
                                                         .count());
}

void Pipeline::VideoCaptureLoop() {
  while (running_.load()) {
    try {
      video_queue_.Push(video_capture_.CaptureOne());
    } catch (const std::exception& ex) {
      logger_.Log(core::LogLevel::kError, std::string("video capture error: ") + ex.what());
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }
}

void Pipeline::AudioCaptureLoop() {
  while (running_.load()) {
    try {
      audio_queue_.Push(audio_capture_.CaptureOne());
    } catch (const std::exception& ex) {
      logger_.Log(core::LogLevel::kError, std::string("audio capture error: ") + ex.what());
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }
}

void Pipeline::EncodeLoop() {
  while (running_.load()) {
    auto video = video_queue_.Pop();
    if (!video.has_value()) {
      break;
    }
    packet_queue_.Push(encoder_.EncodeVideo(*video));

    auto audio = audio_queue_.Pop();
    if (!audio.has_value()) {
      break;
    }
    packet_queue_.Push(encoder_.EncodeAudio(*audio));
  }
}

}  // namespace smgw::app
