# 豆包分析器负载均衡设置指南

本指南将帮助您设置多个API服务实例并配置NGINX负载均衡，以提高系统并发处理能力。

## 1. 启动多个API服务实例

使用提供的脚本可以方便地启动多个API服务实例：

```bash
# 给脚本添加执行权限
chmod +x start_api_services.sh
chmod +x stop_api_services.sh

# 启动所有API服务实例（分别在8080、8081、8082、8083端口）
./start_api_services.sh

# 停止所有API服务实例
./stop_api_services.sh
```

## 2. 安装NGINX

如果尚未安装NGINX，可以使用以下命令安装：

```bash
# Ubuntu/Debian系统
sudo apt update
sudo apt install nginx

# CentOS/RHEL系统
sudo yum install epel-release
sudo yum install nginx
```

## 3. 配置NGINX负载均衡

将提供的配置文件复制到NGINX配置目录：

```bash
# 复制配置文件
sudo cp nginx_config.conf /etc/nginx/sites-available/doubao_analyzer

# 创建软链接启用配置
sudo ln -s /etc/nginx/sites-available/doubao_analyzer /etc/nginx/sites-enabled/

# 如果已有默认配置，可以禁用
# sudo rm /etc/nginx/sites-enabled/default

# 测试配置文件语法
sudo nginx -t

# 重启NGINX
sudo systemctl restart nginx
```

## 4. 配置防火墙

如果启用了防火墙，确保允许HTTP流量：

```bash
# Ubuntu/Debian系统
sudo ufw allow 'Nginx HTTP'

# CentOS/RHEL系统
sudo firewall-cmd --permanent --add-service=http
sudo firewall-cmd --reload
```

## 5. 测试负载均衡

启动所有服务后，您可以通过以下方式测试负载均衡是否正常工作：

```bash
# 检查API服务是否正常运行
curl http://localhost

# 查看NGINX日志
sudo tail -f /var/log/nginx/doubao_analyzer.access.log
sudo tail -f /var/log/nginx/doubao_analyzer.error.log

# 查看API服务日志
tail -f ./logs/api_8080.log
tail -f ./logs/api_8081.log
tail -f ./logs/api_8082.log
tail -f ./logs/api_8083.log
```

## 6. 高级配置选项

### 6.1 使用最少连接负载均衡算法

编辑NGINX配置文件，修改`upstream`部分：

```nginx
upstream doubao_backend {
    least_conn;  # 使用最少连接算法
    server 127.0.0.1:8080;
    server 127.0.0.1:8081;
    server 127.0.0.1:8082;
    server 127.0.0.1:8083;
}
```

### 6.2 基于IP的会话保持

编辑NGINX配置文件，修改`upstream`部分：

```nginx
upstream doubao_backend {
    ip_hash;  # 基于客户端IP的哈希
    server 127.0.0.1:8080;
    server 127.0.0.1:8081;
    server 127.0.0.1:8082;
    server 127.0.0.1:8083;
}
```

### 6.3 配置权重

编辑NGINX配置文件，修改`upstream`部分：

```nginx
upstream doubao_backend {
    server 127.0.0.1:8080 weight=1;  # 权重为1
    server 127.0.0.1:8081 weight=2;  # 权重为2，接收双倍请求
    server 127.0.0.1:8082 weight=1;
    server 127.0.0.1:8083 weight=1;
}
```

## 7. 监控与维护

### 7.1 检查服务状态

```bash
# 检查NGINX状态
sudo systemctl status nginx

# 检查API服务进程
ps aux | grep doubao_api_server

# 查看端口占用情况
netstat -tulpn | grep :80
```

### 7.2 日志管理

```bash
# 查看NGINX日志
sudo tail -f /var/log/nginx/doubao_analyzer.access.log
sudo tail -f /var/log/nginx/doubao_analyzer.error.log

# 查看API服务日志
tail -f ./logs/api_8080.log
```

## 8. 故障排除

### 8.1 端口占用问题

如果遇到"Address already in use"错误：

```bash
# 查找占用端口的进程
sudo lsof -i :8080

# 终止进程
sudo kill -9 <PID>
```

### 8.2 NGINX配置错误

如果NGINX启动失败：

```bash
# 检查配置文件语法
sudo nginx -t

# 查看详细错误信息
sudo journalctl -u nginx
```

## 9. 性能优化建议

1. 根据服务器资源调整API服务实例数量
2. 监控每个API服务实例的负载情况
3. 考虑使用缓存减少后端压力
4. 根据业务需求调整NGINX参数（如缓冲区大小、超时时间等）
5. 考虑使用容器化部署（如Docker）简化部署和管理

## 10. 安全建议

1. 配置防火墙规则限制访问
2. 考虑使用HTTPS加密通信
3. 定期更新系统和软件包
4. 限制请求速率防止DDoS攻击
