cmake_minimum_required(VERSION 3.16)
project(SecureMediaGateway VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SMGW_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(SMGW_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Dependencies)

add_library(smgw_core
  src/core/Logger.cpp
  src/core/Fd.cpp
)

target_include_directories(smgw_core PUBLIC include)
target_link_libraries(smgw_core PUBLIC Threads::Threads)

add_library(smgw_pipeline
  src/concurrency/RingBuffer.cpp
  src/capture/V4L2Capture.cpp
  src/capture/AlsaCapture.cpp
  src/codec/FfmpegEncoder.cpp
  src/streaming/EpollRtspServer.cpp
  src/business/SQLiteEventStore.cpp
  src/app/Pipeline.cpp
)

target_include_directories(smgw_pipeline PUBLIC include)
target_link_libraries(smgw_pipeline
  PUBLIC
    smgw_core
    Threads::Threads
    ${FFMPEG_LIBRARIES}
    ${ALSA_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

target_compile_definitions(smgw_pipeline PUBLIC
  ${FFMPEG_DEFINITIONS}
  ${ALSA_DEFINITIONS}
  ${SQLITE3_DEFINITIONS}
)

add_executable(smgw_gateway src/app/main.cpp)
target_link_libraries(smgw_gateway PRIVATE smgw_pipeline)

if(SMGW_ENABLE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address)
endif()

if(SMGW_ENABLE_TSAN)
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread)
endif()
