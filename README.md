# SecureMediaGateway（基于嵌入式 Linux 的智能安防音视频流媒体网关）

本仓库提供一个**企业级工程骨架**，围绕安防 IPC 的核心业务流设计：

1. V4L2/ALSA 实时采集。
2. FFmpeg H.264/AAC 编码。
3. Epoll ET 高并发 RTSP 服务。
4. SQLite3 事件日志与检索。

## 1. 系统分层架构

```text
+-------------------------------------------------------------+
| Business & DB Layer                                         |
|  - SQLiteEventStore: 事件落盘、检索接口                     |
+-------------------------------------------------------------+
| Streaming Layer                                              |
|  - EpollRtspServer: ET 事件循环、连接管理、媒体广播         |
+-------------------------------------------------------------+
| Codec Layer                                                  |
|  - FfmpegEncoder: H.264 / AAC 编码上下文管理（RAII）        |
+-------------------------------------------------------------+
| Capture Layer                                                |
|  - V4L2Capture / AlsaCapture: 原始视频和音频采集            |
+-------------------------------------------------------------+
| Foundation                                                   |
|  - Logger(异步日志), Fd(RAII), RingBuffer(跨线程解耦)        |
+-------------------------------------------------------------+
```

## 2. 工程特性

- **RAII 资源管理**：Socket FD、数据库连接、编码器上下文在析构中严格释放。
- **线程解耦**：采集线程、编码线程、网络发送线程分离，环形缓冲传递 `shared_ptr`。
- **错误处理**：系统调用失败抛异常，顶层统一兜底。
- **可扩展性**：按模块划分，便于后续接入硬件编码器、检测算法、鉴权能力。

## 3. 目录结构

```text
include/gateway/
  app/               # Pipeline 与媒体数据定义
  business/          # SQLite 业务事件
  capture/           # V4L2/ALSA 采集
  codec/             # FFmpeg 编码
  concurrency/       # RingBuffer
  core/              # Logger / Fd 等基础设施
  streaming/         # Epoll RTSP 服务
src/gateway/...      # 对应实现
```

## 4. 构建依赖

- CMake >= 3.16
- C++17 编译器（gcc/clang）
- pkg-config
- ffmpeg dev 包：`libavcodec libavformat libavutil libswresample libswscale`
- `alsa-lib` dev
- `sqlite3` dev

Ubuntu 示例：

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config \
  libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev \
  libasound2-dev libsqlite3-dev
```

## 5. 编译

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

可执行文件：`build/smgw_gateway`

## 6. 运行

```bash
./build/smgw_gateway
```

默认监听：`0.0.0.0:8554`。

## 7. 下一阶段工业化增强建议

1. **真实零拷贝链路**：使用 V4L2 MMAP/DMABUF，把 buffer 直接交给编码器输入。
2. **硬件编码**：替换 FFmpeg 软编为平台硬编（如 MPP/NVENC/QSV/V4L2 M2M）。
3. **标准 RTSP 会话**：补全 OPTIONS/DESCRIBE/SETUP/PLAY/TEARDOWN 状态机。
4. **鉴权与审计**：RTSP 鉴权（Digest）、操作日志和审计 ID。
5. **可观测性**：导出 Prometheus 指标（帧率、码率、排队时延、丢包率）。
6. **稳定性**：故障自恢复、健康检查、看门狗、配置热加载。

## 8. 代码风格

- 遵循 Google C++ Style Guide。
- 模块接口尽量面向抽象，保持单一职责。
- 避免裸指针拥有语义，资源必须有清晰 owner。
