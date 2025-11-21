#!/bin/bash

# Ollama T4 GPU诊断和优化脚本

# 设置颜色输出
RED='[0;31m'
GREEN='[0;32m'
YELLOW='[1;33m'
NC='[0m' # No Color

echo -e "${GREEN}=== T4 GPU诊断和优化脚本 ===${NC}"

# 1. 检查NVIDIA驱动和CUDA
echo -e "${YELLOW}1. 检查NVIDIA驱动和CUDA...${NC}"
if command -v nvidia-smi &> /dev/null; then
    echo -e "${GREEN}✓ nvidia-smi 可用${NC}"
    nvidia-smi
else
    echo -e "${RED}✗ nvidia-smi 不可用，请检查NVIDIA驱动安装${NC}"
    exit 1
fi

# 2. 检查CUDA环境
echo -e "${YELLOW}2. 检查CUDA环境...${NC}"
if [ -n "$CUDA_VISIBLE_DEVICES" ]; then
    echo -e "${GREEN}✓ CUDA_VISIBLE_DEVICES 已设置: $CUDA_VISIBLE_DEVICES${NC}"
else
    echo -e "${YELLOW}! 设置CUDA_VISIBLE_DEVICES=0${NC}"
    export CUDA_VISIBLE_DEVICES=0
fi

# 3. 检查Docker环境（如果在Docker中）
if [ -f /.dockerenv ]; then
    echo -e "${YELLOW}3. 检测到Docker环境...${NC}"
    if ! ls /dev/nvidia* 1> /dev/null 2>&1; then
        echo -e "${RED}✗ Docker容器中无法访问NVIDIA设备${NC}"
        echo -e "${YELLOW}请确保使用 --gpus all 参数启动Docker容器${NC}"
        exit 1
    else
        echo -e "${GREEN}✓ Docker容器中可以访问NVIDIA设备${NC}"
    fi
fi

# 4. 设置环境变量
echo -e "${YELLOW}4. 设置Ollama环境变量...${NC}"
export OLLAMA_HOST=0.0.0.0:11434
export OLLAMA_NUM_PARALLEL=4
export OLLAMA_MAX_LOADED_MODELS=1
export OLLAMA_MAX_QUEUE=1024
export OLLAMA_KEEP_ALIVE=15m
export OLLAMA_LOAD_TIMEOUT=10m
export OLLAMA_FLASH_ATTENTION=true
export OLLAMA_CONTEXT_LENGTH=2048
export OLLAMA_GPU_OVERHEAD=0
export OLLAMA_DEBUG=INFO
export OLLAMA_SCHED_SPREAD=true
export OLLAMA_KV_CACHE_TYPE=f16
export OLLAMA_LLM_LIBRARY=cuda
export CUDA_VISIBLE_DEVICES=0  # 显式设置

# 5. 启动Ollama
echo -e "${YELLOW}5. 启动Ollama服务...${NC}"
echo "配置参数:"
echo "- 并行处理数: $OLLAMA_NUM_PARALLEL"
echo "- 最大队列: $OLLAMA_MAX_QUEUE"
echo "- Flash Attention: $OLLAMA_FLASH_ATTENTION"
echo "- 上下文长度: $OLLAMA_CONTEXT_LENGTH"
echo "- KV缓存类型: $OLLAMA_KV_CACHE_TYPE"
echo "- CUDA设备: $CUDA_VISIBLE_DEVICES"

# 尝试启动Ollama
ollama serve
