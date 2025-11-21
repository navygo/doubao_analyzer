#!/bin/bash

# 系统优化脚本，针对T4 GPU和qwen3-vl:8b模型优化
# 适用于生产环境批量分析任务

echo "开始系统优化..."

# 1. 优化系统参数
echo "1. 优化系统参数..."
# 增加文件描述符限制
echo "* soft nofile 65536" >> /etc/security/limits.conf
echo "* hard nofile 65536" >> /etc/security/limits.conf

# 优化网络参数
echo "net.core.rmem_max = 134217728" >> /etc/sysctl.conf
echo "net.core.wmem_max = 134217728" >> /etc/sysctl.conf
echo "net.ipv4.tcp_rmem = 4096 87380 134217728" >> /etc/sysctl.conf
echo "net.ipv4.tcp_wmem = 4096 65536 134217728" >> /etc/sysctl.conf

# 应用系统参数
sysctl -p

# 2. 优化GPU设置
echo "2. 优化GPU设置..."
# 设置GPU性能模式
nvidia-smi -pm 1
# 设置GPU时钟
nvidia-smi -ac 877,1215

# 3. 优化Docker环境（如果使用）
if command -v docker &> /dev/null; then
    echo "3. 优化Docker环境..."
    # 增加Docker容器资源限制
    echo '{"default-runtime":"nvidia","runtimes":{"nvidia":{"path":"nvidia-container-runtime","runtimeArgs":[]}}}' > /etc/docker/daemon.json
    systemctl restart docker
fi

# 4. 创建优化的Ollama服务
echo "4. 创建优化的Ollama服务..."
cat > /etc/systemd/system/ollama.service << EOL
[Unit]
Description=Ollama Service
After=network.target

[Service]
Environment="OLLAMA_NUM_PARALLEL=4"
Environment="OLLAMA_MAX_LOADED_MODELS=1"
Environment="OLLAMA_MAX_QUEUE=1024"
Environment="OLLAMA_KEEP_ALIVE=10m"
Environment="OLLAMA_FLASH_ATTENTION=true"
Environment="OLLAMA_CONTEXT_LENGTH=2048"
Environment="OLLAMA_DEBUG=INFO"
ExecStart=/usr/local/bin/ollama serve
Restart=always
User=root

[Install]
WantedBy=multi-user.target
EOL

# 重新加载systemd并启用服务
systemctl daemon-reload
systemctl enable ollama

echo "系统优化完成！"
echo "请重启系统以使所有更改生效，然后运行: systemctl start ollama"
