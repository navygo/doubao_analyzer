#!/bin/bash

echo "=========================================="
echo "豆包分析器负载均衡诊断脚本"
echo "=========================================="
echo

# 1. 检查NGINX是否运行
echo "1. 检查NGINX状态..."
if systemctl is-active --quiet nginx; then
    echo "✅ NGINX正在运行"
else
    echo "❌ NGINX未运行，尝试启动..."
    sudo systemctl start nginx
    if systemctl is-active --quiet nginx; then
        echo "✅ NGINX已成功启动"
    else
        echo "❌ NGINX启动失败"
        echo "错误信息："
        sudo journalctl -u nginx --no-pager -n 20
        exit 1
    fi
fi
echo

# 2. 检查NGINX配置
echo "2. 检查NGINX配置..."
sudo nginx -t
if [ $? -eq 0 ]; then
    echo "✅ NGINX配置正确"
else
    echo "❌ NGINX配置有误"
    exit 1
fi
echo

# 3. 检查API服务实例
echo "3. 检查API服务实例..."
PORTS=("8080" "8081" "8082" "8083")
ALL_RUNNING=true

for port in "${PORTS[@]}"; do
    if curl -s -o /dev/null -w "%{http_code}" http://localhost:$port/health | grep -q "200"; then
        echo "✅ 端口 $port 上的API服务正常运行"
    else
        echo "❌ 端口 $port 上的API服务未响应"
        ALL_RUNNING=false
    fi
done

if [ "$ALL_RUNNING" = false ]; then
    echo
    echo "尝试启动所有API服务实例..."
    ./start_api_services.sh
    sleep 5

    echo "再次检查API服务实例..."
    for port in "${PORTS[@]}"; do
        if curl -s -o /dev/null -w "%{http_code}" http://localhost:$port/health | grep -q "200"; then
            echo "✅ 端口 $port 上的API服务现在正常运行"
        else
            echo "❌ 端口 $port 上的API服务仍然未响应"
        fi
    done
fi
echo

# 4. 检查NGINX配置是否已应用
echo "4. 检查NGINX配置是否已应用..."
if [ -f /etc/nginx/sites-enabled/doubao_analyzer ]; then
    echo "✅ doubao_analyzer配置已启用"
else
    echo "❌ doubao_analyzer配置未启用"
    echo "尝试启用配置..."
    sudo ln -s /home/whj00/doubao_analyzer/nginx_config.conf /etc/nginx/sites-enabled/doubao_analyzer
    sudo nginx -t && sudo systemctl reload nginx
fi

# 检查端口冲突
echo "检查端口冲突..."
conflict_found=false
for config in /etc/nginx/sites-enabled/*; do
    if [ -f "$config" ] && [ "$(basename "$config")" != "doubao_analyzer" ]; then
        if grep -q "listen 80" "$config"; then
            echo "⚠️ 发现冲突配置: $config (监听80端口)"
            conflict_found=true
        fi
    fi
done

if [ "$conflict_found" = true ]; then
    echo "建议运行修复脚本解决冲突: ./fix_nginx_config.sh"
fi
echo

# 5. 测试负载均衡
echo "5. 测试负载均衡..."
for i in {1..5}; do
    echo -n "请求 $i: "
    curl -s -w "状态码: %{http_code}, 响应时间: %{time_total}s\n" http://localhost/api/analyze -o /dev/null
done
echo

# 6. 查看最近的NGINX错误日志
echo "6. 最近的NGINX错误日志:"
sudo tail -n 10 /var/log/nginx/doubao_analyzer.error.log
echo

echo "=========================================="
echo "诊断完成"
echo "=========================================="
