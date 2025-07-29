#!/bin/bash
set -e

APP_NAME=rtsp_crop_streamer

echo "[INFO] 🔧 Building project..."



cd ..
sudo rm -rf build
mkdir -p build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-aarch64.cmake ..
make -j$(nproc)
adb push rtsp_crop_streamer /home/orangepi/test

echo "[INFO] ✅ Build complete."

echo "[INFO] 🚀 Starting RTSP Crop Streamer..."
cd ..
export LD_LIBRARY_PATH=/usr/local/lib:/usr/lib:$LD_LIBRARY_PATH

./build/$APP_NAME 2>&1 | tee run.log
``

