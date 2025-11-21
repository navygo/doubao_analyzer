#!/bin/bash

# 强制重置GPU环境并重启Ollama

# 设置颜色输出
RED='[0;31m'
GREEN='[0;32m'
YELLOW='[1;33m'
NC='[0m' # No Color

echo -e "${GREEN}=== 强制重置GPU环境 ===${NC}"

# 1. 停止所有相关进程
echo -e "${YELLOW}1. 停止所有相关进程...${NC}"
# 停止Ollama服务
systemctl stop ollama || true
pkill -f ollama || true
pkill -f doubao_api_server || true

# 等待进程完全停止
sleep 5

# 2. 强制释放GPU
echo -e "${YELLOW}2. 强制释放GPU...${NC}"
# 查找使用GPU的进程并终止
for pid in $(nvidia-smi pmon -c 1 -s u -o csv,noheader,nounits | awk '{if($2!="-") print $2}' | sort -u); do
    if [ -n "$pid" ] && [ "$pid" != "PID" ]; then
        echo "终止GPU进程: $pid"
        kill -9 $pid 2>/dev/null || true
    fi
done

# 等待进程完全停止
sleep 5

# 3. 卸载NVIDIA内核模块
echo -e "${YELLOW}3. 卸载NVIDIA内核模块...${NC}"
# 尝试卸载模块，忽略错误
rmmod nvidia_uvm 2>/dev/null || true
rmmod nvidia_drm 2>/dev/null || true
rmmod nvidia_modeset 2>/dev/null || true
rmmod nvidia 2>/dev/null || true

# 等待模块卸载
sleep 5

# 4. 重新加载NVIDIA内核模块
echo -e "${YELLOW}4. 重新加载NVIDIA内核模块...${NC}"
modprobe nvidia
modprobe nvidia_modeset
modprobe nvidia_drm
modprobe nvidia_uvm

# 等待模块加载
sleep 5

# 5. 验证GPU状态
echo -e "${YELLOW}5. 验证GPU状态...${NC}"
if nvidia-smi; then
    echo -e "${GREEN}✓ GPU状态正常${NC}"
else
    echo -e "${RED}✗ GPU状态异常${NC}"
    exit 1
fi

# 6. 设置环境变量
echo -e "${YELLOW}6. 设置环境变量...${NC}"
export CUDA_VISIBLE_DEVICES=0
export NVIDIA_VISIBLE_DEVICES=all
export GPU_DEVICE_ORDINAL=0

# 7. 创建优化的systemd服务
echo -e "${YELLOW}7. 创建优化的systemd服务...${NC}"
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
Environment="OLLAMA_DEBUG=INFO"
Environment="OLLAMA_SCHED_SPREAD=true"
Environment="OLLAMA_KV_CACHE_TYPE=f16"
Environment="OLLAMA_LLM_LIBRARY=cuda"
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
sleep 5
if curl -s http://127.0.0.1:11434/api/tags | grep -q "qwen3-vl:8b"; then
    echo -e "${GREEN}✓ Ollama服务已启动${NC}"
else
    echo -e "${YELLOW}! Ollama服务可能未完全启动，等待更长时间...${NC}"
    sleep 10
fi

# 12. 检查Ollama日志
echo -e "${YELLOW}12. 检查Ollama日志...${NC}"
journalctl -u ollama --no-pager -n 50

echo -e "${GREEN}GPU重置完成！${NC}"
echo -e "${YELLOW}如果Ollama仍然无法检测到GPU，请尝试:${NC}"
echo "1. 重启系统"
echo "2. 更新NVIDIA驱动"
echo "3. 重新安装Ollama"
