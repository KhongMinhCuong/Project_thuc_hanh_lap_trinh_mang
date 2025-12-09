#!/bin/bash
# Script để chạy Client (yêu cầu môi trường X11)

echo "[Script] Building client..."
cd "$(dirname "$0")/Client"
mkdir -p build
cd build
cmake .. > /dev/null 2>&1
cmake --build . -j$(nproc)

if [ $? -eq 0 ]; then
    echo "[Script] Build successful! Starting client..."
    ./FileClient
else
    echo "[Script] Build failed!"
    exit 1
fi
