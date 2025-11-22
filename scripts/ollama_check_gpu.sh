#!/bin/bash

# Ollama状态检查脚本

echo "=== Ollama 服务状态检查 ==="
echo "检查时间: $(date)"
echo ""

# 1. 检查 Ollama 进程状态
echo "1. Ollama 进程状态:"
ps aux | grep ollama | grep -v grep
echo ""

# 2. 检查端口监听状态
echo "2. 端口监听状态:"
netstat -tulpn | grep :11434
echo ""

# 3. 检查 GPU 使用情况
echo "3. GPU 使用情况:"
if command -v nvidia-smi &> /dev/null; then
    nvidia-smi
    echo ""
    echo "GPU 进程详情:"
    nvidia-smi pmon -s u -o T
else
    echo "未检测到 NVIDIA GPU 或驱动"
fi
echo ""

# 4. 检查系统资源使用
echo "4. 系统资源使用:"
echo "CPU 使用率:"
top -bn1 | grep "Cpu(s)" | awk '{print $2}' | awk -F'%' '{print $1}'
echo "内存使用情况:"
free -h
echo ""

# 5. 检查 Ollama 模型状态
echo "5. Ollama 模型状态:"
ollama list
echo ""
echo "当前运行的模型:"
ollama ps
echo ""

# 6. 检查 Ollama 环境变量
echo "6. Ollama 环境变量:"
env | grep OLLAMA
echo ""

# 7. 检查最近的 Ollama 日志
echo "7. 最近的 Ollama 日志:"
journalctl -u ollama --since "5 minutes ago" --no-pager
echo ""

# 8. 测试 Ollama API 响应
echo "8. API 响应测试:"
echo "测试连接..."
curl -s http://localhost:11434/api/tags | jq '.models[] | {name: .name, size: .size}' 2>/dev/null || echo "API 连接测试失败"
echo ""

# 9. 检查磁盘使用情况
echo "9. 磁盘使用情况:"
df -h | grep -E "(Filesystem|/dev/)"
echo ""

# 10. 检查网络连接
echo "10. 网络连接统计:"
ss -s | grep -E "(TCP|UDP)"
echo ""

echo "=== 检查完成 ==="
