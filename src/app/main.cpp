#include "gateway/app/Pipeline.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <exception>
#include <iostream>
#include <thread>

namespace {
std::atomic<bool> g_terminated{false};

void HandleSignal(int) { g_terminated.store(true); }
}  // namespace

int main() {
  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  smgw::app::PipelineConfig config;
  smgw::app::Pipeline pipeline(config);

  try {
    pipeline.Start();
    while (!g_terminated.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    pipeline.Stop();
  } catch (const std::exception& ex) {
    std::cerr << "Fatal error: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
