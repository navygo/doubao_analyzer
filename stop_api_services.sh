#!/bin/bash

# 设置端口列表
PORTS=("8080" "8081" "8082" "8083")

# 设置日志目录
LOG_DIR="./logs"

# 停止API服务
for port in "${PORTS[@]}"; do
    if [ -f "$LOG_DIR/api_$port.pid" ]; then
        PID=$(cat "$LOG_DIR/api_$port.pid")
        echo "Stopping API service on port $port (PID: $PID)..."
        kill $PID
        rm "$LOG_DIR/api_$port.pid"
    else
        echo "No PID file found for API service on port $port"
    fi
done

echo "All API services stopped."
