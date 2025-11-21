#!/bin/bash

# GPU环境检查脚本

# 设置颜色输出
RED='[0;31m'
GREEN='[0;32m'
YELLOW='[1;33m'
NC='[0m' # No Color

echo -e "${GREEN}=== GPU环境检查 ===${NC}"

# 1. 检查NVIDIA驱动
echo -e "${YELLOW}1. 检查NVIDIA驱动...${NC}"
if command -v nvidia-smi &> /dev/null; then
    echo -e "${GREEN}✓ nvidia-smi 可用${NC}"
    nvidia-smi
    echo ""

    # 获取GPU信息
    GPU_NAME=$(nvidia-smi --query-gpu=name --format=csv,noheader,nounits | head -n 1)
    echo -e "${GREEN}GPU型号: $GPU_NAME${NC}"

    # 检查是否为T4
    if [[ "$GPU_NAME" == *"Tesla T4"* ]]; then
        echo -e "${GREEN}✓ 检测到Tesla T4 GPU${NC}"
    else
        echo -e "${YELLOW}! 未检测到Tesla T4，当前GPU: $GPU_NAME${NC}"
    fi
else
    echo -e "${RED}✗ nvidia-smi 不可用，请检查NVIDIA驱动安装${NC}"
    exit 1
fi

# 2. 检查CUDA安装
echo -e "${YELLOW}2. 检查CUDA安装...${NC}"
if command -v nvcc &> /dev/null; then
    CUDA_VERSION=$(nvcc --version | grep "release" | awk '{print $6}' | cut -c2-)
    echo -e "${GREEN}✓ CUDA版本: $CUDA_VERSION${NC}"
else
    echo -e "${YELLOW}! nvcc 不可用，可能只安装了CUDA运行时${NC}"
fi

# 检查CUDA库
if ldconfig -p | grep -q libcuda; then
    echo -e "${GREEN}✓ CUDA库已安装${NC}"
else
    echo -e "${RED}✗ CUDA库未找到${NC}"
fi

# 3. 检查Docker环境
echo -e "${YELLOW}3. 检查Docker环境...${NC}"
if [ -f /.dockerenv ]; then
    echo -e "${GREEN}✓ 在Docker容器中运行${NC}"

    # 检查NVIDIA容器运行时
    if command -v nvidia-container-cli &> /dev/null; then
        echo -e "${GREEN}✓ NVIDIA容器运行时可用${NC}"
    else
        echo -e "${YELLOW}! NVIDIA容器运行时不可用${NC}"
    fi

    # 检查GPU设备
    if ls /dev/nvidia* 1> /dev/null 2>&1; then
        echo -e "${GREEN}✓ 可以访问NVIDIA设备${NC}"
        ls -la /dev/nvidia*
    else
        echo -e "${RED}✗ 无法访问NVIDIA设备${NC}"
        echo -e "${YELLOW}请使用 --gpus all 参数启动Docker容器${NC}"
    fi
else
    echo -e "${GREEN}✓ 在主机环境中运行${NC}"
fi

# 4. 检查Ollama GPU支持
echo -e "${YELLOW}4. 检查Ollama GPU支持...${NC}"
if command -v ollama &> /dev/null; then
    echo -e "${GREEN}✓ Ollama已安装${NC}"

    # 尝试获取GPU信息
    if ollama list 2>&1 | grep -q "GPU"; then
        echo -e "${GREEN}✓ Ollama支持GPU${NC}"
    else
        echo -e "${YELLOW}! 无法确认Ollama GPU支持${NC}"
    fi
else
    echo -e "${RED}✗ Ollama未安装${NC}"
fi

# 5. 环境变量检查
echo -e "${YELLOW}5. 检查环境变量...${NC}"
if [ -n "$CUDA_VISIBLE_DEVICES" ]; then
    echo -e "${GREEN}✓ CUDA_VISIBLE_DEVICES: $CUDA_VISIBLE_DEVICES${NC}"
else
    echo -e "${YELLOW}! CUDA_VISIBLE_DEVICES 未设置${NC}"
fi

if [ -n "$NVIDIA_VISIBLE_DEVICES" ]; then
    echo -e "${GREEN}✓ NVIDIA_VISIBLE_DEVICES: $NVIDIA_VISIBLE_DEVICES${NC}"
else
    echo -e "${YELLOW}! NVIDIA_VISIBLE_DEVICES 未设置${NC}"
fi

# 6. 生成修复建议
echo -e "${YELLOW}6. 修复建议...${NC}"
echo -e "${GREEN}如果Ollama无法检测到GPU，请尝试以下步骤:${NC}"
echo "1. 确保使用最新版本的Ollama"
echo "2. 设置环境变量: export CUDA_VISIBLE_DEVICES=0"
echo "3. 如果在Docker中，使用 --gpus all 参数启动容器"
echo "4. 重启系统或Docker服务"
echo "5. 检查NVIDIA驱动是否兼容当前CUDA版本"

echo -e "${GREEN}检查完成！${NC}"
