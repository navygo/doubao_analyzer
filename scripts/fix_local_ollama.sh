#!/bin/bash

# 修复本地Ollama GPU检测问题

# 设置颜色输出
RED='[0;31m'
GREEN='[0;32m'
YELLOW='[1;33m'
NC='[0m' # No Color

echo -e "${GREEN}=== 修复本地Ollama GPU检测问题 ===${NC}"

# 1. 停止Ollama服务
echo -e "${YELLOW}1. 停止Ollama服务...${NC}"
systemctl stop ollama || true
pkill -f ollama || true

# 2. 备份原始Ollama二进制文件
echo -e "${YELLOW}2. 备份原始Ollama二进制文件...${NC}"
if [ -f /usr/local/bin/ollama ]; then
    cp /usr/local/bin/ollama /usr/local/bin/ollama.backup
    echo -e "${GREEN}✓ 已备份原始Ollama二进制文件${NC}"
else
    echo -e "${YELLOW}! 未找到Ollama二进制文件${NC}"
fi

# 3. 重新安装Ollama
echo -e "${YELLOW}3. 重新安装Ollama...${NC}"
curl -fsSL https://ollama.com/install.sh | sh

# 4. 创建优化的systemd服务
echo -e "${YELLOW}4. 创建优化的systemd服务...${NC}"
cat > /etc/systemd/system/ollama.service << EOF
[Unit]
Description=Ollama Service
After=network.target nvidia-udev.service

[Service]
Environment="CUDA_VISIBLE_DEVICES=0"
Environment="NVIDIA_VISIBLE_DEVICES=all"
Environment="GPU_DEVICE_ORDINAL=0"
Environment="OLLAMA_HOST=0.0.0.0:11434"
Environment="OLLAMA_NUM_PARALLEL=4"
Environment="OLLAMA_MAX_LOADED_MODELS=1"
Environment="OLLAMA_MAX_QUEUE=1024"
Environment="OLLAMA_KEEP_ALIVE=15m"
Environment="OLLAMA_LOAD_TIMEOUT=10m"
Environment="OLLAMA_FLASH_ATTENTION=true"
Environment="OLLAMA_CONTEXT_LENGTH=2048"
Environment="OLLAMA_GPU_OVERHEAD=0"
Environment="OLLAMA_DEBUG=DEBUG"  # 设置为DEBUG以获取更多信息
Environment="OLLAMA_SCHED_SPREAD=true"
Environment="OLLAMA_KV_CACHE_TYPE=f16"
Environment="OLLAMA_LLM_LIBRARY=cuda"
Environment="OLLAMA_MAX_QUEUE=1024"
ExecStart=/usr/local/bin/ollama serve
Restart=always
RestartSec=10
User=root

[Install]
WantedBy=multi-user.target
EOF

# 重新加载systemd
systemctl daemon-reload
systemctl enable ollama

# 5. 检查NVIDIA驱动和CUDA
echo -e "${YELLOW}5. 检查NVIDIA驱动和CUDA...${NC}"
if nvidia-smi; then
    echo -e "${GREEN}✓ NVIDIA驱动正常${NC}"
else
    echo -e "${RED}✗ NVIDIA驱动异常${NC}"
    exit 1
fi

# 6. 设置环境变量
echo -e "${YELLOW}6. 设置环境变量...${NC}"
cat > /etc/profile.d/ollama.sh << EOF
export CUDA_VISIBLE_DEVICES=0
export NVIDIA_VISIBLE_DEVICES=all
export GPU_DEVICE_ORDINAL=0
EOF

# 7. 创建Ollama用户目录
echo -e "${YELLOW}7. 创建Ollama用户目录...${NC}"
mkdir -p /root/.ollama

# 8. 启动Ollama
echo -e "${YELLOW}8. 启动Ollama...${NC}"
systemctl start ollama

# 等待服务启动
sleep 10

# 9. 检查服务状态
echo -e "${YELLOW}9. 检查服务状态...${NC}"
systemctl status ollama

# 10. 检查GPU使用情况
echo -e "${YELLOW}10. 检查GPU使用情况...${NC}"
nvidia-smi

# 11. 测试GPU检测
echo -e "${YELLOW}11. 测试GPU检测...${NC}"
# 记录初始GPU内存
INITIAL_GPU_MEMORY=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits)
echo "初始GPU内存使用: ${INITIAL_GPU_MEMORY} MiB"

# 加载模型
echo "正在加载qwen3-vl:8b模型..."
timeout 30 ollama run qwen3-vl:8b "Hello" > /tmp/model_output.log 2>&1 &
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
    echo -e "${GREEN}✓ Ollama正在使用GPU${NC}"
else
    echo -e "${RED}✗ 模型未加载到GPU内存${NC}"
    echo -e "${YELLOW}! 请尝试使用Docker运行Ollama${NC}"
fi

# 12. 检查Ollama日志
echo -e "${YELLOW}12. 检查Ollama日志...${NC}"
journalctl -u ollama --no-pager -n 50 | grep -i "gpu\|cuda\|vram"

# 清理
kill $OLLAMA_PID 2>/dev/null || true

echo -e "${GREEN}本地Ollama修复完成！${NC}"
echo -e "${YELLOW}如果问题仍然存在，请尝试Docker方案: ./docker_gpu_ollama.sh${NC}"
