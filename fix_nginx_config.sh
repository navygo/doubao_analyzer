#!/bin/bash

echo "=========================================="
echo "修复NGINX配置脚本"
echo "=========================================="
echo

# 1. 备份当前配置
echo "1. 备份当前NGINX配置..."
sudo mkdir -p /etc/nginx/backup
sudo cp -r /etc/nginx/sites-enabled/* /etc/nginx/sites-available/* /etc/nginx/backup/ 2>/dev/null
echo "✅ 配置已备份到 /etc/nginx/backup/"
echo

# 2. 禁用默认配置
echo "2. 禁用默认配置..."
# 删除所有可能的默认配置文件
sudo rm -f /etc/nginx/sites-enabled/default
sudo rm -f /etc/nginx/sites-enabled/defaultbak
sudo rm -f /etc/nginx/sites-enabled/default.conf

# 检查sites-enabled目录中的所有配置文件，确保没有重复的监听端口
echo "检查可能的端口冲突..."
for config in /etc/nginx/sites-enabled/*; do
    if [ -f "$config" ]; then
        if grep -q "listen 80" "$config" && [ "$(basename "$config")" != "doubao_analyzer" ]; then
            echo "发现冲突配置: $config"
            sudo mv "$config" "/etc/nginx/sites-available/$(basename "$config").bak"
            echo "✅ 已移动冲突配置到备份"
        fi
    fi
done
echo

# 3. 应用我们的配置
echo "3. 应用豆包分析器配置..."
sudo cp /home/whj00/doubao_analyzer/nginx_config.conf /etc/nginx/sites-available/doubao_analyzer
sudo ln -sf /etc/nginx/sites-available/doubao_analyzer /etc/nginx/sites-enabled/doubao_analyzer
echo "✅ 配置已应用"
echo

# 4. 测试配置
echo "4. 测试NGINX配置..."
if sudo nginx -t; then
    echo "✅ 配置测试通过"
else
    echo "❌ 配置测试失败，恢复备份..."
    sudo rm /etc/nginx/sites-enabled/doubao_analyzer
    sudo cp -r /etc/nginx/backup/* /etc/nginx/sites-enabled/ 2>/dev/null
    sudo cp -r /etc/nginx/backup/* /etc/nginx/sites-available/ 2>/dev/null
    echo "❌ 已恢复备份配置"
    exit 1
fi
echo

# 5. 重新加载NGINX
echo "5. 重新加载NGINX..."
sudo systemctl reload nginx
if systemctl is-active --quiet nginx; then
    echo "✅ NGINX已成功重新加载"
else
    echo "❌ NGINX重新加载失败"
    sudo systemctl status nginx
    exit 1
fi
echo

# 6. 测试负载均衡
echo "6. 测试负载均衡..."
sleep 2
for i in {1..3}; do
    echo -n "请求 $i: "
    response=$(curl -s -w "状态码: %{http_code}, 响应时间: %{time_total}s" http://localhost/api/analyze)
    echo "$response"
done
echo

echo "=========================================="
echo "修复完成"
echo "=========================================="
echo
echo "如果问题仍然存在，请运行诊断脚本："
echo "./diagnose_load_balancer.sh"
