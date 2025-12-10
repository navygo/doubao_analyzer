#!/bin/bash

# 设置API服务可执行文件路径
API_EXECUTABLE="./build/doubao_api_server"

# 设置端口列表
PORTS=("8080" "8081" "8082" "8083")

# 设置日志目录
LOG_DIR="./logs"

# 创建日志目录（如果不存在）
mkdir -p $LOG_DIR

# 启动API服务
for port in "${PORTS[@]}"; do
    echo "Starting API service on port $port..."
    nohup $API_EXECUTABLE --port $port > "$LOG_DIR/api_$port.log" 2>&1 &
    echo $! > "$LOG_DIR/api_$port.pid"
done

echo "All API services started."
