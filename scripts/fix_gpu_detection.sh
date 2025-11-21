#!/bin/bash

# 修复Ollama GPU检测问题脚本

# 设置颜色输出
RED='[0;31m'
GREEN='[0;32m'
YELLOW='[1;33m'
NC='[0m' # No Color

echo -e "${GREEN}=== 修复Ollama GPU检测问题 ===${NC}"

# 1. 停止Ollama服务
echo -e "${YELLOW}1. 停止Ollama服务...${NC}"
pkill ollama || true
sleep 3

# 2. 设置环境变量
echo -e "${YELLOW}2. 设置GPU环境变量...${NC}"
export CUDA_VISIBLE_DEVICES=0
export NVIDIA_VISIBLE_DEVICES=all
export GPU_DEVICE_ORDINAL=0

# 3. 检查NVIDIA驱动
echo -e "${YELLOW}3. 检查NVIDIA驱动状态...${NC}"
if command -v nvidia-smi &> /dev/null; then
    echo -e "${GREEN}✓ NVIDIA驱动正常${NC}"
    nvidia-smi -L
else
    echo -e "${RED}✗ NVIDIA驱动异常${NC}"
    exit 1
fi

# 4. 检查Docker环境
if [ -f /.dockerenv ]; then
    echo -e "${YELLOW}4. Docker环境GPU设备检查...${NC}"
    if ! ls /dev/nvidia* 1> /dev/null 2>&1; then
        echo -e "${RED}✗ Docker容器中无法访问GPU设备${NC}"
        echo -e "${YELLOW}请使用以下命令重新启动Docker容器:${NC}"
        echo "docker run --gpus all -v /path/to/data:/data your-image"
        exit 1
    else
        echo -e "${GREEN}✓ Docker容器中GPU设备可访问${NC}"
    fi
fi

# 5. 重置NVIDIA内核模块
echo -e "${YELLOW}5. 重置NVIDIA内核模块...${NC}"
if [ "$(id -u)" = "0" ]; then
    echo "以root权限运行，尝试重置NVIDIA内核模块..."
    rmmod nvidia_uvm || true
    rmmod nvidia_drm || true
    rmmod nvidia_modeset || true
    rmmod nvidia || true
    sleep 2
    modprobe nvidia
    modprobe nvidia_modeset
    modprobe nvidia_drm
    modprobe nvidia_uvm
    echo -e "${GREEN}✓ NVIDIA内核模块已重置${NC}"
else
    echo -e "${YELLOW}! 非root权限，跳过内核模块重置${NC}"
fi

# 6. 设置Ollama环境变量
echo -e "${YELLOW}6. 设置Ollama环境变量...${NC}"
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

# 7. 创建systemd服务文件（如果是root用户）
if [ "$(id -u)" = "0" ]; then
    echo -e "${YELLOW}7. 创建Ollama systemd服务...${NC}"
    cat > /etc/systemd/system/ollama.service << EOF
[Unit]
Description=Ollama Service
After=network.target

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
User=root

[Install]
WantedBy=multi-user.target
EOF

    systemctl daemon-reload
    systemctl enable ollama
    echo -e "${GREEN}✓ Ollama systemd服务已创建${NC}"
fi

# 8. 启动Ollama
echo -e "${YELLOW}8. 启动Ollama服务...${NC}"
if [ "$(id -u)" = "0" ]; then
    systemctl start ollama
    sleep 5
    systemctl status ollama
else
    ollama serve &
    OLLAMA_PID=$!
    sleep 5
    ps aux | grep ollama
fi

# 9. 测试GPU检测
echo -e "${YELLOW}9. 测试GPU检测...${NC}"
sleep 5
if curl -s http://127.0.0.1:11434/api/tags | grep -q "qwen3-vl:8b"; then
    echo -e "${GREEN}✓ Ollama服务已启动${NC}"
else
    echo -e "${YELLOW}! Ollama服务可能未完全启动，等待更长时间...${NC}"
    sleep 10
fi

# 10. 检查GPU使用情况
echo -e "${YELLOW}10. 检查GPU使用情况...${NC}"
if command -v nvidia-smi &> /dev/null; then
    nvidia-smi
fi

echo -e "${GREEN}GPU检测修复完成！${NC}"
echo -e "${YELLOW}如果Ollama仍然无法检测到GPU，请检查:${NC}"
echo "1. NVIDIA驱动版本是否兼容"
echo "2. CUDA版本是否正确安装"
echo "3. 系统是否有足够的内存"
echo "4. 是否有其他进程占用GPU"
