#!/bin/bash

# 简单的Ollama GPU启动脚本

# 设置颜色输出
RED='[0;31m'
GREEN='[0;32m'
YELLOW='[1;33m'
NC='[0m' # No Color

echo -e "${GREEN}=== 简单Ollama GPU启动脚本 ===${NC}"

# 1. 停止所有相关进程
echo -e "${YELLOW}1. 停止所有相关进程...${NC}"
systemctl stop ollama || true
pkill -f ollama || true

# 等待进程完全停止
sleep 5

# 2. 设置环境变量
echo -e "${YELLOW}2. 设置环境变量...${NC}"
export CUDA_VISIBLE_DEVICES=0
export NVIDIA_VISIBLE_DEVICES=all
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

# 3. 验证GPU状态
echo -e "${YELLOW}3. 验证GPU状态...${NC}"
if nvidia-smi; then
    echo -e "${GREEN}✓ GPU状态正常${NC}"
else
    echo -e "${RED}✗ GPU状态异常${NC}"
    exit 1
fi

# 4. 直接启动Ollama
echo -e "${YELLOW}4. 直接启动Ollama...${NC}"
echo "配置参数:"
echo "- CUDA设备: $CUDA_VISIBLE_DEVICES"
echo "- NVIDIA设备: $NVIDIA_VISIBLE_DEVICES"
echo "- 并行处理数: $OLLAMA_NUM_PARALLEL"
echo "- Flash Attention: $OLLAMA_FLASH_ATTENTION"
echo "- KV缓存类型: $OLLAMA_KV_CACHE_TYPE"

# 直接启动Ollama，不使用systemd
ollama serve
