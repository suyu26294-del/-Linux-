#pragma once

#include "gateway/app/MediaTypes.h"
#include "gateway/core/Fd.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace smgw::streaming {

class EpollRtspServer {
 public:
  EpollRtspServer(std::string bind_ip, uint16_t port);
  ~EpollRtspServer();

  void Start();
  void Stop();
  void Broadcast(const app::EncodedPacketPtr& packet);

 private:
  void EventLoop();
  void HandleAccept();
  void HandleReadable(int client_fd);
  void HandleWritable(int client_fd);

  std::string bind_ip_;
  uint16_t port_;
  core::Fd listen_fd_;
  core::Fd epoll_fd_;
  std::atomic<bool> running_{false};
  std::thread worker_;
  std::mutex clients_mutex_;
  std::unordered_map<int, std::vector<uint8_t>> pending_write_;
};

}  // namespace smgw::streaming
