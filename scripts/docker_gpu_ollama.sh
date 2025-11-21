#!/bin/bash

# 使用Docker运行Ollama，确保GPU访问

# 设置颜色输出
RED='[0;31m'
GREEN='[0;32m'
YELLOW='[1;33m'
NC='[0m' # No Color

echo -e "${GREEN}=== 使用Docker运行Ollama ===${NC}"

# 1. 停止当前Ollama服务
echo -e "${YELLOW}1. 停止当前Ollama服务...${NC}"
systemctl stop ollama || true
pkill -f ollama || true

# 2. 检查Docker是否安装
echo -e "${YELLOW}2. 检查Docker是否安装...${NC}"
if ! command -v docker &> /dev/null; then
    echo -e "${RED}✗ Docker未安装${NC}"
    echo "请先安装Docker: https://docs.docker.com/get-docker/"
    exit 1
else
    echo -e "${GREEN}✓ Docker已安装${NC}"
fi

# 3. 检查NVIDIA Container Toolkit
echo -e "${YELLOW}3. 检查NVIDIA Container Toolkit...${NC}"
if ! docker run --rm --gpus all nvidia/cuda:12.2.0-base-ubuntu20.04 nvidia-smi &> /dev/null; then
    echo -e "${RED}✗ NVIDIA Container Toolkit未安装或配置不正确${NC}"
    echo "请安装NVIDIA Container Toolkit: https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html"
    exit 1
else
    echo -e "${GREEN}✓ NVIDIA Container Toolkit正常工作${NC}"
fi

# 4. 停止并删除现有容器
echo -e "${YELLOW}4. 停止并删除现有容器...${NC}"
docker stop ollama-gpu 2>/dev/null || true
docker rm ollama-gpu 2>/dev/null || true

# 5. 创建Ollama数据卷
echo -e "${YELLOW}5. 创建Ollama数据卷...${NC}"
docker volume create ollama-data 2>/dev/null || true

# 6. 启动Ollama Docker容器
echo -e "${YELLOW}6. 启动Ollama Docker容器...${NC}"
docker run -d     --name ollama-gpu     --gpus all     -v ollama-data:/root/.ollama     -p 11434:11434     -e OLLAMA_NUM_PARALLEL=4     -e OLLAMA_MAX_LOADED_MODELS=1     -e OLLAMA_MAX_QUEUE=1024     -e OLLAMA_KEEP_ALIVE=15m     -e OLLAMA_LOAD_TIMEOUT=10m     -e OLLAMA_FLASH_ATTENTION=true     -e OLLAMA_CONTEXT_LENGTH=2048     -e OLLAMA_GPU_OVERHEAD=0     -e OLLAMA_DEBUG=INFO     -e OLLAMA_SCHED_SPREAD=true     -e OLLAMA_KV_CACHE_TYPE=f16     -e OLLAMA_LLM_LIBRARY=cuda     ollama/ollama

# 7. 等待容器启动
echo -e "${YELLOW}7. 等待容器启动...${NC}"
sleep 10

# 8. 检查容器状态
echo -e "${YELLOW}8. 检查容器状态...${NC}"
docker ps | grep ollama-gpu

# 9. 检查容器日志
echo -e "${YELLOW}9. 检查容器日志...${NC}"
docker logs ollama-gpu | tail -20

# 10. 测试API连接
echo -e "${YELLOW}10. 测试API连接...${NC}"
if curl -s http://127.0.0.1:11434/api/tags > /dev/null; then
    echo -e "${GREEN}✓ API连接正常${NC}"
else
    echo -e "${RED}✗ API连接失败${NC}"
    exit 1
fi

# 11. 测试GPU使用
echo -e "${YELLOW}11. 测试GPU使用...${NC}"
# 记录初始GPU内存
INITIAL_GPU_MEMORY=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits)
echo "初始GPU内存使用: ${INITIAL_GPU_MEMORY} MiB"

# 加载模型
echo "正在加载qwen3-vl:8b模型..."
docker exec ollama-gpu ollama run qwen3-vl:8b "Hello" &
OLLAMA_PID=$!

# 等待模型加载
sleep 20

# 检查GPU内存使用
FINAL_GPU_MEMORY=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits)
echo "最终GPU内存使用: ${FINAL_GPU_MEMORY} MiB"

# 计算内存差异
MEMORY_DIFF=$((FINAL_GPU_MEMORY - INITIAL_GPU_MEMORY))
echo "模型使用的GPU内存: ${MEMORY_DIFF} MiB"

# 分析结果
if [ $MEMORY_DIFF -gt 100 ]; then
    echo -e "${GREEN}✓ 模型已加载到GPU内存${NC}"
    echo -e "${GREEN}✓ Docker中的Ollama正在使用GPU${NC}"
else
    echo -e "${RED}✗ 模型未加载到GPU内存${NC}"
fi

# 清理
kill $OLLAMA_PID 2>/dev/null || true

echo -e "${GREEN}Docker Ollama设置完成！${NC}"
echo -e "${YELLOW}要查看容器日志，请运行: docker logs -f ollama-gpu${NC}"
echo -e "${YELLOW}要停止容器，请运行: docker stop ollama-gpu${NC}"
