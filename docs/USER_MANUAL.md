# 豆包媒体分析系统使用手册

## 目录

1. [系统概述](#系统概述)
2. [系统安装与部署](#系统安装与部署)
3. [系统配置](#系统配置)
4. [API接口使用指南](#api接口使用指南)
5. [常见问题与解决方案](#常见问题与解决方案)
6. [维护与监控](#维护与监控)

## 系统概述

豆包媒体分析系统是基于字节跳动豆包大模型的图片和视频分析工具，使用C++17实现，支持Ubuntu系统部署，并提供数据库存储功能。系统提供RESTful API接口，可供其他系统调用，实现图片和视频的智能分析功能。

### 主要功能

- 🖼️ **图片分析**: 支持常见图片格式 (JPG, PNG, BMP, WebP等)
- 🎬 **视频分析**: 支持多种视频格式 (MP4, AVI, MOV, MKV等)
- 📁 **批量处理**: 支持批量分析多个媒体文件
- 🏷️ **智能标签**: 自动从分析结果中提取标签
- 💾 **数据库支持**: 支持MySQL数据库存储分析结果
- 🔍 **结果查询**: 支持按条件查询和标签查询数据库记录
- 🌐 **RESTful API**: 提供简单的HTTP接口
- 🔐 **JWT认证**: 基于JWT的身份认证和授权机制

## 系统安装与部署

### 系统要求

- Ubuntu 18.04 或更高版本
- C++17 兼容编译器 (GCC 7+)
- CMake 3.10+
- MySQL 5.7+ (如需使用数据库功能)
- 至少 4GB 内存
- 至少 10GB 可用磁盘空间

### 一键部署

```bash
# 给脚本执行权限
chmod +x setup.sh install_deps.sh

# 运行部署脚本
./setup.sh
```

### 手动部署步骤

#### 1. 安装依赖
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libopencv-dev libcurl4-openssl-dev nlohmann-json3-dev libmysqlclient-dev ffmpeg
```

#### 2. 编译项目
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### 3. 安装到系统
```bash
sudo make install
```

#### 4. 验证安装
```bash
doubao_analyzer --help
```

#### 5. 测试安装
```bash
# 运行功能测试
cd build
./test_config

# 测试API连接 (需要有效API密钥)
doubao_analyzer --api-key YOUR_KEY --image test/test.jpg

# 测试数据库功能 (需要配置数据库)
doubao_analyzer --api-key YOUR_KEY --image test/test.jpg --save-to-db
```

## 系统配置

### API密钥配置

豆包媒体分析系统需要有效的豆包API密钥才能正常工作。您可以通过以下方式配置API密钥：

1. **命令行参数** (临时使用):
   ```bash
   doubao_api_server --api-key YOUR_API_KEY
   ```

2. **环境变量** (推荐):
   ```bash
   export DOUBAO_API_KEY=YOUR_API_KEY
   ```

3. **配置文件** (永久配置):
   创建 `~/.doubao_analyzer/config.json` 文件:
   ```json
   {
     "api_key": "YOUR_API_KEY"
   }
   ```

### 数据库配置

#### 1. 安装MySQL
```bash
sudo apt update
sudo apt install mysql-server
```

#### 2. 创建数据库和用户
```sql
CREATE DATABASE doubao_analyzer;
CREATE USER 'doubao_user'@'localhost' IDENTIFIED BY 'your_password';
GRANT ALL PRIVILEGES ON doubao_analyzer.* TO 'doubao_user'@'localhost';
FLUSH PRIVILEGES;
```

#### 3. 配置数据库连接
编辑配置文件(默认位置: `~/.doubao_analyzer/config.json`):
```json
{
  "database": {
    "host": "localhost",
    "user": "doubao_user",
    "password": "your_password",
    "database": "doubao_analyzer",
    "port": 3306,
    "charset": "utf8mb4",
    "connection_timeout": 60,
    "read_timeout": 60,
    "write_timeout": 60
  },
  "auth": {
    "admin_user": "admin",
    "admin_pass": "admin123"
  }
}
```

### API服务器配置

#### 启动API服务器
```bash
# 基本用法
doubao_api_server --api-key YOUR_API_KEY

# 自定义端口和主机
doubao_api_server --api-key YOUR_API_KEY --port 8080 --host 0.0.0.0

# 查看帮助
doubao_api_server --help
```

#### 生产环境部署

对于生产环境，建议使用systemd管理服务：

1. 创建服务文件 `/etc/systemd/system/doubao-api.service`:
   ```ini
   [Unit]
   Description=Doubao Media Analysis API Server
   After=network.target mysql.service

   [Service]
   Type=simple
   User=doubao
   WorkingDirectory=/opt/doubao_analyzer
   ExecStart=/usr/local/bin/doubao_api_server --api-key YOUR_API_KEY --port 8080
   Restart=always
   RestartSec=10

   [Install]
   WantedBy=multi-user.target
   ```

2. 启用并启动服务:
   ```bash
   sudo systemctl enable doubao-api
   sudo systemctl start doubao-api
   ```

## API接口使用指南

### 认证机制

本服务使用基于 `Bearer JWT` 的认证方案，支持完整的用户注册、登录和管理功能。

- 使用管理员账号密码登录获取访问令牌
- 访问令牌有效期15分钟
- 刷新令牌有效期7天
- 令牌验证失败返回401 Unauthorized
- 用户名密码错误返回401 Unauthorized

#### 用户登录 - POST /api/auth

用户登录获取访问令牌和刷新令牌。

**请求示例:**
```json
{
    "username": "admin",
    "password": "admin123"
}
```

**响应示例:**
```json
{
    "success": true,
    "message": "登录成功",
    "data": {
        "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
        "expires_in": 900,
        "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
        "refresh_expires_in": 604800
    }
}
```

#### 令牌刷新 - POST /api/auth/refresh

使用刷新令牌获取新的访问令牌和刷新令牌。

**请求示例:**
```json
{
    "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**响应示例:**
```json
{
    "success": true,
    "message": "刷新成功",
    "data": {
        "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
        "expires_in": 900,
        "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
        "refresh_expires_in": 604800
    }
}
```

#### 使用令牌调用受保护接口

在HTTP头中加入Authorization：
```
Authorization: Bearer <access_token>
```

### 分析接口

#### 单媒体分析 - POST /api/analyze

分析图片或视频内容。

**请求示例:**
```json
{
    "media_type": "image",
    "media_url": "https://example.com/image.jpg",
    "prompt": "请分析这张图片的内容",
    "max_tokens": 1500,
    "save_to_db": true
}
```

**响应示例:**
```json
{
    "success": true,
    "message": "图片分析成功",
    "response_time": 2.3,
    "data": {
        "content": "这张图片展示了一座美丽的山峰...",
        "tags": ["山峰", "自然", "风景"],
        "response_time": 2.1,
        "usage": {
            "prompt_tokens": 120,
            "completion_tokens": 150,
            "total_tokens": 270
        },
        "saved_to_db": true
    }
}
```

#### 批量媒体分析 - POST /api/batch_analyze

批量分析多个图片或视频。

**请求示例:**
```json
{
    "requests": [
        {
            "media_type": "image",
            "media_url": "https://example.com/image1.jpg",
            "prompt": "请分析这张图片的内容",
            "max_tokens": 1500,
            "save_to_db": true
        },
        {
            "media_type": "video",
            "media_url": "https://example.com/video1.mp4",
            "prompt": "请分析这段视频的内容",
            "max_tokens": 2000,
            "video_frames": 8,
            "save_to_db": true
        }
    ]
}
```

**响应示例:**
```json
{
    "success": true,
    "message": "批量分析完成，成功: 2/2",
    "response_time": 15.7,
    "data": {
        "results": [
            {
                "task_id": "batch_0_1689427345",
                "success": true,
                "content": "这张图片展示了一座美丽的山峰...",
                "tags": ["山峰", "自然", "风景"],
                "response_time": 2.1,
                "usage": {
                    "prompt_tokens": 120,
                    "completion_tokens": 150,
                    "total_tokens": 270
                }
            },
            {
                "task_id": "batch_1_1689427345",
                "success": true,
                "content": "这段视频展示了一群人在公园里...",
                "tags": ["人物", "公园", "活动"],
                "response_time": 12.3,
                "usage": {
                    "prompt_tokens": 180,
                    "completion_tokens": 350,
                    "total_tokens": 530
                }
            }
        ],
        "summary": {
            "total": 2,
            "successful": 2,
            "failed": 0
        },
        "timing": {
            "total_seconds": 15.7,
            "pending_tasks": 0,
            "active_threads": 4
        }
    }
}
```

### 查询接口

#### 查询分析结果 - POST /api/query

查询已分析的结果记录，支持多种查询方式。

**请求示例:**
```json
{
    "query_type": "tag",
    "tag": "风景"
}
```

**响应示例:**
```json
{
    "success": true,
    "message": "查询成功，共找到 5 条记录",
    "response_time": 0.2,
    "data": {
        "results": [
            {
                "id": 1,
                "file_path": "https://example.com/image1.jpg",
                "file_name": "image1.jpg",
                "file_type": "image",
                "analysis_result": "这张图片展示了一座美丽的山峰...",
                "tags": "山峰, 自然, 风景",
                "response_time": 2.1,
                "created_at": "2023-07-15 14:30:22"
            },
            {
                "id": 3,
                "file_path": "https://example.com/image2.jpg",
                "file_name": "image2.jpg",
                "file_type": "image",
                "analysis_result": "这是一片宁静的湖泊，周围环绕着青山...",
                "tags": "湖泊, 自然, 风景",
                "response_time": 1.8,
                "created_at": "2023-07-15 15:45:10"
            }
        ],
        "count": 5
    }
}
```

#### 服务器状态 - GET /api/status

获取服务器状态和数据库统计信息。

**响应示例:**
```json
{
    "success": true,
    "message": "服务器状态查询成功",
    "response_time": 0.0,
    "data": {
        "server_status": "running",
        "api_key_set": true,
        "port": 8080,
        "host": "0.0.0.0",
        "database_stats": {
            "total_records": 125,
            "image_records": 78,
            "video_records": 47,
            "last_updated": "2023-07-15 16:20:05"
        }
    }
}
```

### 请求参数说明

#### 分析接口参数

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| media_type | string | 是 | 媒体类型，"image"或"video" |
| media_url | string | 是 | 图片或视频的URL地址 |
| prompt | string | 否 | 自定义提示词，留空则使用默认提示词 |
| max_tokens | int | 否 | 最大令牌数，图片默认1500，视频默认2000 |
| video_frames | int | 否 | 视频提取帧数，仅视频分析有效，默认为5 |
| save_to_db | bool | 否 | 是否将结果保存到数据库，默认为true |

#### 查询接口参数

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| query_type | string | 是 | 查询类型："all", "tag", "type", "date_range", "recent", "url" |
| tag | string | 否 | 要查询的标签（当query_type为"tag"时使用） |
| file_type | string | 否 | 要查询的文件类型（当query_type为"type"时使用） |
| start_date | string | 否 | 开始日期（当query_type为"date_range"时使用） |
| end_date | string | 否 | 结束日期（当query_type为"date_range"时使用） |
| limit | int | 否 | 返回结果数量限制（当query_type为"recent"时使用，默认10） |
| condition | string | 否 | 自定义查询条件（当query_type为"all"时使用） |
| media_url | string | 否 | 要查询的媒体URL（当query_type为"url"时使用） |

### 查询类型说明

- **all**: 查询所有结果，可使用condition参数添加自定义条件
- **tag**: 根据标签查询，需要提供tag参数
- **type**: 根据文件类型查询（"image"或"video"），需要提供file_type参数
- **date_range**: 根据日期范围查询，需要提供start_date和end_date参数
- **recent**: 获取最近的记录，可使用limit参数限制返回数量
- **url**: 根据媒体URL查询，需要提供media_url参数

## 常见问题与解决方案

### 认证错误处理

#### 常见认证错误

| 错误 | 原因 | 解决方案 |
|------|------|----------|
| 401 Unauthorized | 令牌缺失或无效 | 检查请求头中是否包含有效的Bearer令牌 |
| 401 Token expired | 令牌已过期 | 使用refresh_token接口刷新令牌 |
| 401 Invalid token | 令牌格式错误 | 重新登录获取新令牌 |
| 403 Forbidden | 权限不足 | 联系管理员分配相应权限 |

#### 令牌刷新流程

1. 客户端检测到令牌即将过期（建议在过期前1小时）
2. 调用`/api/auth/refresh`接口，传入当前令牌
3. 获取新令牌并更新本地存储
4. 使用新令牌继续请求API

### 常见问题

- **连接失败**: 检查API密钥是否正确，网络是否通畅
- **下载失败**: 确认URL是否可访问，文件是否过大
- **数据库错误**: 检查数据库配置是否正确，MySQL服务是否运行
- **端口占用**: 使用--port参数指定其他端口
- **查询无结果**: 检查查询条件是否正确，确认数据库中是否有匹配记录
- **认证失败**: 检查用户名密码是否正确，令牌是否在有效期内
- **JWT错误**: 检查JWT密钥配置是否正确，令牌格式是否有效

## 维护与监控

### 日志管理

API服务器会将日志输出到标准输出，建议使用日志管理系统收集和分析日志。

### 性能监控

1. 使用`/api/status`接口定期检查服务器状态
2. 监控系统资源使用情况（CPU、内存、磁盘）
3. 监控数据库性能和连接数

### 数据库维护

1. 定期备份数据库：
   ```bash
   ./scripts/backup_database.sh
   ```

2. 恢复数据库（如需要）：
   ```bash
   ./scripts/restore_database.sh backup_file.sql
   ```

3. 优化数据库性能：
   ```sql
   OPTIMIZE TABLE media_analysis;
   ```

### 系统更新

1. 停止服务：
   ```bash
   sudo systemctl stop doubao-api
   ```

2. 更新代码：
   ```bash
   git pull origin main
   ```

3. 重新编译和安装：
   ```bash
   cd build
   make clean
   cmake ..
   make -j$(nproc)
   sudo make install
   ```

4. 重启服务：
   ```bash
   sudo systemctl start doubao-api
   ```

### 安全建议

1. 使用HTTPS保护API通信
2. 定期更换管理员密码
3. 限制API访问IP范围
4. 定期更新系统和依赖库
5. 监控异常访问和请求
6. 设置合理的请求速率限制
