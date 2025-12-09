#!/bin/bash
# Script để chạy Server

echo "[Script] Building server..."
cd "$(dirname "$0")"
mkdir -p build
cd build
cmake .. > /dev/null 2>&1
cmake --build . -j$(nproc)

if [ $? -eq 0 ]; then
    echo "[Script] Build successful! Starting server..."
    ./FileServer
else
    echo "[Script] Build failed!"
    exit 1
fi
