#!/bin/bash

# 安装基本依赖
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    libopencv-dev \
    libcurl4-openssl-dev \
    nlohmann-json3-dev

# 如果系统版本较老，可能需要手动安装nlohmann-json
if ! dpkg -l | grep -q nlohmann-json3-dev; then
    echo "手动安装nlohmann-json..."
    wget https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
    sudo mkdir -p /usr/include/nlohmann
    sudo mv json.hpp /usr/include/nlohmann/
fi

echo "✅ 依赖安装完成"
