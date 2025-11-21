#!/bin/bash

# 测试Ollama GPU使用情况

# 设置颜色输出
RED='[0;31m'
GREEN='[0;32m'
YELLOW='[1;33m'
NC='[0m' # No Color

echo -e "${GREEN}=== 测试Ollama GPU使用情况 ===${NC}"

# 1. 记录初始GPU状态
echo -e "${YELLOW}1. 记录初始GPU状态...${NC}"
nvidia-smi
INITIAL_GPU_MEMORY=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits)
echo "初始GPU内存使用: ${INITIAL_GPU_MEMORY} MiB"

# 2. 确保Ollama正在运行
echo -e "${YELLOW}2. 确保Ollama正在运行...${NC}"
if ! systemctl is-active --quiet ollama; then
    echo -e "${RED}✗ Ollama服务未运行${NC}"
    exit 1
else
    echo -e "${GREEN}✓ Ollama服务正在运行${NC}"
fi

# 3. 测试API连接
echo -e "${YELLOW}3. 测试API连接...${NC}"
if curl -s http://127.0.0.1:11434/api/tags > /dev/null; then
    echo -e "${GREEN}✓ API连接正常${NC}"
else
    echo -e "${RED}✗ API连接失败${NC}"
    exit 1
fi

# 4. 加载模型并监控GPU使用
echo -e "${YELLOW}4. 加载模型并监控GPU使用...${NC}"
echo "正在后台启动GPU监控..."
nvidia-smi dmon -s u -o DT -c 30 > /tmp/gpu_monitor.log 2>&1 &
GPU_MONITOR_PID=$!

echo "正在加载qwen3-vl:8b模型..."
timeout 30 ollama run qwen3-vl:8b "Hello" > /tmp/model_output.log 2>&1 &
OLLAMA_PID=$!

# 等待模型加载
sleep 20

# 5. 检查GPU使用情况
echo -e "${YELLOW}5. 检查GPU使用情况...${NC}"
nvidia-smi
FINAL_GPU_MEMORY=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits)
echo "最终GPU内存使用: ${FINAL_GPU_MEMORY} MiB"

# 计算内存差异
MEMORY_DIFF=$((FINAL_GPU_MEMORY - INITIAL_GPU_MEMORY))
echo "模型使用的GPU内存: ${MEMORY_DIFF} MiB"

# 6. 分析结果
echo -e "${YELLOW}6. 分析结果...${NC}"
if [ $MEMORY_DIFF -gt 100 ]; then
    echo -e "${GREEN}✓ 模型已加载到GPU内存${NC}"
    echo -e "${GREEN}✓ Ollama正在使用GPU${NC}"
else
    echo -e "${RED}✗ 模型未加载到GPU内存${NC}"
    echo -e "${RED}✗ Ollama可能在使用CPU${NC}"
fi

# 7. 检查Ollama日志
echo -e "${YELLOW}7. 检查Ollama日志...${NC}"
journalctl -u ollama --no-pager -n 50 | grep -i "gpu\|cuda\|vram"

# 8. 清理
echo -e "${YELLOW}8. 清理...${NC}"
kill $GPU_MONITOR_PID 2>/dev/null || true
kill $OLLAMA_PID 2>/dev/null || true

# 显示GPU监控日志
echo -e "${YELLOW}GPU监控日志:${NC}"
cat /tmp/gpu_monitor.log

echo -e "${GREEN}测试完成！${NC}"
