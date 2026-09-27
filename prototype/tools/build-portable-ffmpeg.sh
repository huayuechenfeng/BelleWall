#!/usr/bin/env bash
# Run from an extracted, unmodified FFmpeg 9.0.1 source directory in MSYS2 UCRT64.
set -euo pipefail
export PATH=/ucrt64/bin:/usr/bin
./configure --prefix="$PWD/bellewall-install" --target-os=mingw32 \
  --disable-autodetect --disable-network --disable-devices \
  --disable-doc --disable-debug --disable-x86asm --disable-ffplay --disable-ffprobe \
  --disable-shared --enable-static --extra-ldflags=-static \
  --disable-encoders --enable-encoder=rawvideo,mpeg4 \
  --disable-muxers --enable-muxer=rawvideo,mp4 \
  --disable-protocols --enable-protocol=file,pipe \
  --disable-filters --enable-filter=scale,crop,pad,fps,setpts,setsar,format,null
make -j"${BELLEWALL_BUILD_JOBS:-4}"
make install
