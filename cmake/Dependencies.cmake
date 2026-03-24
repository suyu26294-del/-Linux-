find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
  libavcodec
  libavformat
  libavutil
  libswresample
  libswscale
)
set(FFMPEG_LIBRARIES PkgConfig::FFMPEG)
set(FFMPEG_DEFINITIONS SMGW_HAVE_FFMPEG=1)

pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
set(ALSA_LIBRARIES PkgConfig::ALSA)
set(ALSA_DEFINITIONS SMGW_HAVE_ALSA=1)

pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)
set(SQLITE3_LIBRARIES PkgConfig::SQLITE3)
set(SQLITE3_DEFINITIONS SMGW_HAVE_SQLITE=1)
