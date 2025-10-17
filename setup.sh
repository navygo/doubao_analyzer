#!/bin/bash

echo "🚀 开始部署豆包媒体分析工具..."

# 检查系统
if [[ "$(lsb_release -is 2>/dev/null)" != "Ubuntu" ]]; then
    echo "⚠️  此脚本专为Ubuntu系统设计"
fi

# 更新系统包
echo "📦 更新系统包..."
sudo apt update
sudo apt upgrade -y

# 安装编译工具
echo "🔧 安装编译工具..."
sudo apt install -y build-essential cmake pkg-config

# 安装OpenCV依赖
echo "📷 安装OpenCV依赖..."
sudo apt install -y libopencv-dev

# 安装CURL开发库
echo "🌐 安装CURL开发库..."
sudo apt install -y libcurl4-openssl-dev

# 安装nlohmann-json
echo "📄 安装JSON库..."
sudo apt install -y nlohmann-json3-dev

# 创建构建目录
mkdir -p build
cd build

# 配置和编译
echo "🔨 编译项目..."
cmake ..
make -j$(nproc)

# 安装到系统
echo "📁 安装到系统..."
sudo make install

echo "✅ 部署完成!"
echo ""
echo "使用示例:"
echo "  doubao_analyzer --api-key YOUR_KEY --image test.jpg"
echo "  doubao_analyzer --api-key YOUR_KEY --video test.mp4"
echo "  doubao_analyzer (进入交互模式)"
