#include "gateway/streaming/EpollRtspServer.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <stdexcept>

namespace smgw::streaming {

namespace {
constexpr int kMaxEvents = 128;

void SetNonBlocking(int fd) {
  const int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    smgw::core::ThrowSystemError("fcntl O_NONBLOCK failed");
  }
}
}  // namespace

EpollRtspServer::EpollRtspServer(std::string bind_ip, uint16_t port)
    : bind_ip_(std::move(bind_ip)), port_(port) {}

EpollRtspServer::~EpollRtspServer() { Stop(); }

void EpollRtspServer::Start() {
  if (running_.exchange(true)) {
    return;
  }

  const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    smgw::core::ThrowSystemError("socket failed");
  }
  listen_fd_.Reset(server_fd);
  SetNonBlocking(listen_fd_.Get());

  int one = 1;
  ::setsockopt(listen_fd_.Get(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port_);
  if (::inet_pton(AF_INET, bind_ip_.c_str(), &addr.sin_addr) <= 0) {
    throw std::runtime_error("Invalid bind IP");
  }

  if (::bind(listen_fd_.Get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    smgw::core::ThrowSystemError("bind failed");
  }

  if (::listen(listen_fd_.Get(), SOMAXCONN) < 0) {
    smgw::core::ThrowSystemError("listen failed");
  }

  const int epfd = ::epoll_create1(0);
  if (epfd < 0) {
    smgw::core::ThrowSystemError("epoll_create1 failed");
  }
  epoll_fd_.Reset(epfd);

  epoll_event ev{};
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = listen_fd_.Get();
  if (::epoll_ctl(epoll_fd_.Get(), EPOLL_CTL_ADD, listen_fd_.Get(), &ev) < 0) {
    smgw::core::ThrowSystemError("epoll_ctl add listen fd failed");
  }

  worker_ = std::thread(&EpollRtspServer::EventLoop, this);
}

void EpollRtspServer::Stop() {
  running_.store(false);
  if (worker_.joinable()) {
    worker_.join();
  }
}

void EpollRtspServer::Broadcast(const app::EncodedPacketPtr& packet) {
  std::lock_guard<std::mutex> lock(clients_mutex_);
  for (auto& [fd, cache] : pending_write_) {
    cache.insert(cache.end(), packet->payload.begin(), packet->payload.end());
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.fd = fd;
    ::epoll_ctl(epoll_fd_.Get(), EPOLL_CTL_MOD, fd, &ev);
  }
}

void EpollRtspServer::EventLoop() {
  std::array<epoll_event, kMaxEvents> events{};
  while (running_.load()) {
    const int n = ::epoll_wait(epoll_fd_.Get(), events.data(), kMaxEvents, 100);
    if (n <= 0) {
      continue;
    }

    for (int i = 0; i < n; ++i) {
      const auto& ev = events[static_cast<size_t>(i)];
      if (ev.data.fd == listen_fd_.Get()) {
        HandleAccept();
      } else {
        if ((ev.events & EPOLLIN) != 0) {
          HandleReadable(ev.data.fd);
        }
        if ((ev.events & EPOLLOUT) != 0) {
          HandleWritable(ev.data.fd);
        }
      }
    }
  }
}

void EpollRtspServer::HandleAccept() {
  while (true) {
    int client_fd = ::accept4(listen_fd_.Get(), nullptr, nullptr, SOCK_NONBLOCK);
    if (client_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
      }
      smgw::core::ThrowSystemError("accept4 failed");
    }

    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = client_fd;
    if (::epoll_ctl(epoll_fd_.Get(), EPOLL_CTL_ADD, client_fd, &ev) < 0) {
      ::close(client_fd);
      continue;
    }

    std::lock_guard<std::mutex> lock(clients_mutex_);
    pending_write_.emplace(client_fd, std::vector<uint8_t>{});
  }
}

void EpollRtspServer::HandleReadable(int client_fd) {
  char buffer[1024];
  const ssize_t n = ::recv(client_fd, buffer, sizeof(buffer), 0);
  if (n <= 0) {
    ::epoll_ctl(epoll_fd_.Get(), EPOLL_CTL_DEL, client_fd, nullptr);
    ::close(client_fd);
    std::lock_guard<std::mutex> lock(clients_mutex_);
    pending_write_.erase(client_fd);
    return;
  }

  constexpr char kRtspResp[] = "RTSP/1.0 200 OK\r\nCSeq: 1\r\n\r\n";
  std::lock_guard<std::mutex> lock(clients_mutex_);
  auto& cache = pending_write_[client_fd];
  cache.insert(cache.end(), kRtspResp, kRtspResp + std::strlen(kRtspResp));
}

void EpollRtspServer::HandleWritable(int client_fd) {
  std::lock_guard<std::mutex> lock(clients_mutex_);
  auto it = pending_write_.find(client_fd);
  if (it == pending_write_.end() || it->second.empty()) {
    return;
  }

  auto& cache = it->second;
  const ssize_t n = ::send(client_fd, cache.data(), cache.size(), MSG_NOSIGNAL);
  if (n <= 0) {
    return;
  }

  cache.erase(cache.begin(), cache.begin() + n);
  if (cache.empty()) {
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = client_fd;
    ::epoll_ctl(epoll_fd_.Get(), EPOLL_CTL_MOD, client_fd, &ev);
  }
}

}  // namespace smgw::streaming
