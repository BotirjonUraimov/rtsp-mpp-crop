#!/bin/bash
set -e

APP_NAME=rtsp_crop_streamer

echo "[INFO] 🔧 Building project..."
if [ ! -d "build" ]; then
    echo "[INFO] Creating build directory..."
else
    echo "[INFO] Build directory already exists, cleaning..."
    rm -rf build/*
fi
mkdir -p build
cd build
cmake ..
make -j$(nproc)

echo "[INFO] ✅ Build complete."

echo "[INFO] 🚀 Starting RTSP Crop Streamer..."
cd ..
export LD_LIBRARY_PATH=/usr/local/lib:/usr/lib:$LD_LIBRARY_PATH

./build/$APP_NAME 2>&1 | tee run.log
``