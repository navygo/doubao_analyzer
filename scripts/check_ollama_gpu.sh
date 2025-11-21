#!/bin/bash

# 检查Ollama GPU检测状态

# 设置颜色输出
RED='[0;31m'
GREEN='[0;32m'
YELLOW='[1;33m'
NC='[0m' # No Color

echo -e "${GREEN}=== 检查Ollama GPU检测状态 ===${NC}"

# 1. 获取Ollama服务状态
echo -e "${YELLOW}1. 检查Ollama服务状态...${NC}"
systemctl status ollama

# 2. 获取完整的Ollama日志
echo -e "${YELLOW}2. 获取完整的Ollama日志...${NC}"
journalctl -u ollama --no-pager -n 100

# 3. 检查GPU使用情况
echo -e "${YELLOW}3. 检查GPU使用情况...${NC}"
nvidia-smi

# 4. 测试Ollama API
echo -e "${YELLOW}4. 测试Ollama API...${NC}"
curl -s http://127.0.0.1:11434/api/tags | jq .

# 5. 尝试加载模型并检查GPU使用
echo -e "${YELLOW}5. 尝试加载模型并检查GPU使用...${NC}"
# 记录当前GPU内存使用
GPU_MEMORY_BEFORE=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits)

echo "加载模型前的GPU内存使用: ${GPU_MEMORY_BEFORE} MiB"

# 加载模型
echo "正在加载qwen3-vl:8b模型..."
timeout 30 ollama run qwen3-vl:8b "Hello" &

# 等待模型加载
sleep 20

# 检查GPU内存使用
GPU_MEMORY_AFTER=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits)
echo "加载模型后的GPU内存使用: ${GPU_MEMORY_AFTER} MiB"

# 计算内存差异
MEMORY_DIFF=$((GPU_MEMORY_AFTER - GPU_MEMORY_BEFORE))
echo "模型使用的GPU内存: ${MEMORY_DIFF} MiB"

if [ $MEMORY_DIFF -gt 100 ]; then
    echo -e "${GREEN}✓ 模型已加载到GPU内存${NC}"
else
    echo -e "${RED}✗ 模型未加载到GPU内存${NC}"
fi

# 6. 检查Ollama进程
echo -e "${YELLOW}6. 检查Ollama进程...${NC}"
ps aux | grep ollama

# 7. 检查环境变量
echo -e "${YELLOW}7. 检查环境变量...${NC}"
systemctl show ollama | grep Environment

echo -e "${GREEN}检查完成！${NC}"
