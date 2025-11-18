# 豆包媒体分析系统API技术文档

## 目录

1. [API概述](#api概述)
2. [认证与授权](#认证与授权)
3. [API接口详解](#api接口详解)
4. [错误处理](#错误处理)
5. [性能优化](#性能优化)
6. [SDK与代码示例](#sdk与代码示例)
7. [集成指南](#集成指南)

## API概述

豆包媒体分析系统提供RESTful API接口，支持图片和视频的智能分析功能。API基于HTTP协议，使用JSON格式进行数据交换，并采用JWT进行身份认证和授权。

### 基础信息

- **基础URL**: `http://your-server:port/api`
- **协议**: HTTP/HTTPS
- **数据格式**: JSON
- **字符编码**: UTF-8
- **认证方式**: Bearer JWT

### API版本控制

当前API版本为v1，所有请求路径均以`/api`开头。未来版本将通过路径前缀进行区分，如`/api/v2`。

## 认证与授权

### JWT令牌机制

系统使用JWT(JSON Web Token)进行身份认证和授权。JWT包含三部分：头部(Header)、载荷(Payload)和签名(Signature)。

#### 令牌类型

1. **访问令牌(Access Token)**:
   - 有效期: 15分钟
   - 用于访问受保护的API接口
   - 在HTTP请求头中通过`Authorization: Bearer <access_token>`传递

2. **刷新令牌(Refresh Token)**:
   - 有效期: 7天
   - 用于获取新的访问令牌
   - 当访问令牌过期时使用

#### 获取令牌

通过`POST /api/auth`接口获取令牌：

```bash
curl -X POST http://your-server:8080/api/auth   -H "Content-Type: application/json"   -d '{
    "username": "admin",
    "password": "admin123"
  }'
```

#### 刷新令牌

通过`POST /api/auth/refresh`接口刷新令牌：

```bash
curl -X POST http://your-server:8080/api/auth/refresh   -H "Content-Type: application/json"   -d '{
    "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
  }'
```

#### 使用令牌

在请求头中添加`Authorization`字段：

```bash
curl -X POST http://your-server:8080/api/analyze   -H "Content-Type: application/json"   -H "Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."   -d '{
    "media_type": "image",
    "media_url": "https://example.com/image.jpg"
  }'
```

### 令牌管理最佳实践

1. **客户端存储**:
   - 访问令牌存储在内存中
   - 刷新令牌可安全地存储在持久化存储中

2. **令牌刷新策略**:
   - 在访问令牌过期前5分钟开始刷新
   - 刷新失败后引导用户重新登录

3. **令牌撤销**:
   - 系统会在以下情况撤销令牌:
     - 用户主动登出
     - 刷新令牌被使用(单次有效)
     - 检测到异常活动

## API接口详解

### 1. 认证接口

#### 1.1 用户登录

- **接口地址**: `POST /api/auth`
- **功能描述**: 用户登录获取访问令牌和刷新令牌
- **是否认证**: 否

**请求参数**:

| 参数名 | 类型 | 必需 | 说明 |
|--------|------|------|------|
| username | string | 是 | 用户名 |
| password | string | 是 | 密码 |

**响应示例**:

```json
{
    "success": true,
    "message": "登录成功",
    "data": {
        "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
        "expires_in": 900,
        "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
        "refresh_expires_in": 604800
    },
    "response_time": 0.05
}
```

#### 1.2 令牌刷新

- **接口地址**: `POST /api/auth/refresh`
- **功能描述**: 使用刷新令牌获取新的访问令牌和刷新令牌
- **是否认证**: 否

**请求参数**:

| 参数名 | 类型 | 必需 | 说明 |
|--------|------|------|------|
| refresh_token | string | 是 | 刷新令牌 |

**响应示例**:

```json
{
    "success": true,
    "message": "刷新成功",
    "data": {
        "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
        "expires_in": 900,
        "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
        "refresh_expires_in": 604800
    },
    "response_time": 0.03
}
```

### 2. 分析接口

#### 2.1 单媒体分析

- **接口地址**: `POST /api/analyze`
- **功能描述**: 分析单个图片或视频
- **是否认证**: 是

**请求参数**:

| 参数名 | 类型 | 必需 | 说明 |
|--------|------|------|------|
| media_type | string | 是 | 媒体类型，"image"或"video" |
| media_url | string | 是 | 媒体文件URL |
| prompt | string | 否 | 自定义提示词，留空则使用默认提示词 |
| max_tokens | int | 否 | 最大令牌数，图片默认1500，视频默认2000 |
| video_frames | int | 否 | 视频提取帧数，仅视频分析有效，默认为5 |
| save_to_db | bool | 否 | 是否将结果保存到数据库，默认为true |

**响应示例**:

```json
{
    "success": true,
    "message": "图片分析成功",
    "response_time": 2.3,
    "data": {
        "content": "这张图片展示了一座美丽的山峰，山上有茂密的森林，天空中有几朵白云...",
        "tags": ["山峰", "森林", "自然", "风景"],
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

#### 2.2 批量媒体分析

- **接口地址**: `POST /api/batch_analyze`
- **功能描述**: 批量分析多个图片或视频
- **是否认证**: 是

**请求参数**:

| 参数名 | 类型 | 必需 | 说明 |
|--------|------|------|------|
| requests | array | 是 | 分析请求对象数组，每个对象包含与单媒体分析相同的参数 |

**响应示例**:

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

### 3. 查询接口

#### 3.1 查询分析结果

- **接口地址**: `POST /api/query`
- **功能描述**: 查询已分析的结果记录，支持多种查询方式
- **是否认证**: 是

**请求参数**:

| 参数名 | 类型 | 必需 | 说明 |
|--------|------|------|------|
| query_type | string | 是 | 查询类型："all", "tag", "type", "date_range", "recent", "url" |
| tag | string | 否 | 要查询的标签（当query_type为"tag"时使用） |
| file_type | string | 否 | 要查询的文件类型（当query_type为"type"时使用） |
| start_date | string | 否 | 开始日期（当query_type为"date_range"时使用） |
| end_date | string | 否 | 结束日期（当query_type为"date_range"时使用） |
| limit | int | 否 | 返回结果数量限制（当query_type为"recent"时使用，默认10） |
| condition | string | 否 | 自定义查询条件（当query_type为"all"时使用） |
| media_url | string | 否 | 要查询的媒体URL（当query_type为"url"时使用） |

**响应示例**:

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

### 4. 状态接口

#### 4.1 服务器状态

- **接口地址**: `GET /api/status`
- **功能描述**: 获取服务器状态和数据库统计信息
- **是否认证**: 否

**响应示例**:

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

## 错误处理

### HTTP状态码

API使用标准的HTTP状态码表示请求结果：

| 状态码 | 说明 |
|--------|------|
| 200 | 请求成功 |
| 400 | 请求参数错误 |
| 401 | 未授权（令牌缺失或无效） |
| 403 | 禁止访问（权限不足） |
| 404 | 资源不存在 |
| 500 | 服务器内部错误 |

### 错误响应格式

所有错误响应都遵循统一的JSON格式：

```json
{
    "success": false,
    "message": "错误描述",
    "error": "错误类型",
    "response_time": 0.05
}
```

### 常见错误类型

| 错误类型 | 说明 | 解决方案 |
|----------|------|----------|
| Unauthorized | 令牌缺失或无效 | 检查请求头中是否包含有效的Bearer令牌 |
| Token expired | 令牌已过期 | 使用refresh_token接口刷新令牌 |
| Invalid token | 令牌格式错误 | 重新登录获取新令牌 |
| Invalid request format | 请求格式错误 | 检查请求参数是否符合API规范 |
| Invalid media type | 无效的媒体类型 | 确保media_type为"image"或"video" |
| Image download failed | 图片下载失败 | 检查图片URL是否可访问 |
| Video analysis failed | 视频分析失败 | 检查视频URL是否可访问，格式是否支持 |
| Database error | 数据库错误 | 检查数据库连接和配置 |
| Query error | 查询错误 | 检查查询参数是否正确 |

## 性能优化

### 请求优化

1. **批量处理**:
   - 使用`/api/batch_analyze`接口一次性处理多个媒体文件
   - 批量请求比多个单独请求效率更高

2. **合理设置参数**:
   - 根据需要设置合适的`max_tokens`值
   - 视频分析时根据视频长度和内容复杂度设置`video_frames`

3. **缓存结果**:
   - 对于相同URL的媒体，可缓存分析结果
   - 使用`/api/query`接口检查是否已有分析结果

### 服务器端优化

1. **并发处理**:
   - API服务器使用多线程处理请求
   - 可通过配置调整工作线程数量

2. **资源管理**:
   - 自动清理临时文件
   - 限制并发分析任务数量

3. **数据库优化**:
   - 合理设计索引
   - 定期优化数据库表

### 客户端优化

1. **异步请求**:
   - 使用异步方式发送API请求
   - 避免阻塞UI线程

2. **重试机制**:
   - 实现合理的重试策略
   - 对网络错误和临时服务不可用进行重试

3. **请求限流**:
   - 控制请求频率，避免服务器过载
   - 实现请求队列管理

## SDK与代码示例

### Python SDK示例

```python
import requests
import json
import time

class DoubaoAnalyzerClient:
    def __init__(self, base_url, username, password):
        self.base_url = base_url
        self.access_token = None
        self.refresh_token = None
        self.token_expires_at = 0

        # 登录获取令牌
        self.login(username, password)

    def login(self, username, password):
        """登录获取访问令牌和刷新令牌"""
        url = f"{self.base_url}/api/auth"
        data = {
            "username": username,
            "password": password
        }

        response = requests.post(url, json=data)
        result = response.json()

        if result["success"]:
            self.access_token = result["data"]["access_token"]
            self.refresh_token = result["data"]["refresh_token"]
            self.token_expires_at = time.time() + result["data"]["expires_in"] - 60  # 提前1分钟刷新
            return True
        return False

    def refresh_access_token(self):
        """刷新访问令牌"""
        url = f"{self.base_url}/api/auth/refresh"
        data = {
            "refresh_token": self.refresh_token
        }

        response = requests.post(url, json=data)
        result = response.json()

        if result["success"]:
            self.access_token = result["data"]["access_token"]
            self.refresh_token = result["data"]["refresh_token"]
            self.token_expires_at = time.time() + result["data"]["expires_in"] - 60  # 提前1分钟刷新
            return True
        return False

    def ensure_valid_token(self):
        """确保令牌有效"""
        if time.time() >= self.token_expires_at:
            return self.refresh_access_token()
        return True

    def analyze_media(self, media_type, media_url, prompt=None, max_tokens=None, video_frames=None, save_to_db=True):
        """分析单个媒体文件"""
        if not self.ensure_valid_token():
            raise Exception("Failed to refresh token")

        url = f"{self.base_url}/api/analyze"
        headers = {
            "Authorization": f"Bearer {self.access_token}",
            "Content-Type": "application/json"
        }

        data = {
            "media_type": media_type,
            "media_url": media_url,
            "save_to_db": save_to_db
        }

        if prompt:
            data["prompt"] = prompt
        if max_tokens:
            data["max_tokens"] = max_tokens
        if video_frames and media_type == "video":
            data["video_frames"] = video_frames

        response = requests.post(url, headers=headers, json=data)
        return response.json()

    def batch_analyze(self, requests):
        """批量分析媒体文件"""
        if not self.ensure_valid_token():
            raise Exception("Failed to refresh token")

        url = f"{self.base_url}/api/batch_analyze"
        headers = {
            "Authorization": f"Bearer {self.access_token}",
            "Content-Type": "application/json"
        }

        data = {
            "requests": requests
        }

        response = requests.post(url, headers=headers, json=data)
        return response.json()

    def query_results(self, query_type, **kwargs):
        """查询分析结果"""
        if not self.ensure_valid_token():
            raise Exception("Failed to refresh token")

        url = f"{self.base_url}/api/query"
        headers = {
            "Authorization": f"Bearer {self.access_token}",
            "Content-Type": "application/json"
        }

        data = {
            "query_type": query_type
        }
        data.update(kwargs)

        response = requests.post(url, headers=headers, json=data)
        return response.json()

    def get_status(self):
        """获取服务器状态"""
        url = f"{self.base_url}/api/status"
        response = requests.get(url)
        return response.json()


# 使用示例
if __name__ == "__main__":
    # 初始化客户端
    client = DoubaoAnalyzerClient(
        base_url="http://localhost:8080",
        username="admin",
        password="admin123"
    )

    # 分析单张图片
    result = client.analyze_media(
        media_type="image",
        media_url="https://example.com/image.jpg",
        prompt="请详细描述这张图片的内容"
    )
    print(json.dumps(result, indent=2))

    # 批量分析
    requests = [
        {
            "media_type": "image",
            "media_url": "https://example.com/image1.jpg"
        },
        {
            "media_type": "video",
            "media_url": "https://example.com/video1.mp4",
            "video_frames": 8
        }
    ]
    batch_result = client.batch_analyze(requests)
    print(json.dumps(batch_result, indent=2))

    # 查询结果
    query_result = client.query_results(
        query_type="tag",
        tag="风景"
    )
    print(json.dumps(query_result, indent=2))
```

### JavaScript SDK示例

```javascript
class DoubaoAnalyzerClient {
    constructor(baseUrl, username, password) {
        this.baseUrl = baseUrl;
        this.accessToken = null;
        this.refreshToken = null;
        this.tokenExpiresAt = 0;

        // 登录获取令牌
        this.login(username, password);
    }

    async login(username, password) {
        const url = `${this.baseUrl}/api/auth`;
        const data = {
            username: username,
            password: password
        };

        try {
            const response = await fetch(url, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(data)
            });

            const result = await response.json();

            if (result.success) {
                this.accessToken = result.data.access_token;
                this.refreshToken = result.data.refresh_token;
                this.tokenExpiresAt = Date.now() + (result.data.expires_in - 60) * 1000; // 提前1分钟刷新
                return true;
            }
            return false;
        } catch (error) {
            console.error('Login error:', error);
            return false;
        }
    }

    async refreshAccessToken() {
        const url = `${this.baseUrl}/api/auth/refresh`;
        const data = {
            refresh_token: this.refreshToken
        };

        try {
            const response = await fetch(url, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(data)
            });

            const result = await response.json();

            if (result.success) {
                this.accessToken = result.data.access_token;
                this.refreshToken = result.data.refresh_token;
                this.tokenExpiresAt = Date.now() + (result.data.expires_in - 60) * 1000; // 提前1分钟刷新
                return true;
            }
            return false;
        } catch (error) {
            console.error('Token refresh error:', error);
            return false;
        }
    }

    async ensureValidToken() {
        if (Date.now() >= this.tokenExpiresAt) {
            return await this.refreshAccessToken();
        }
        return true;
    }

    async analyzeMedia(mediaType, mediaUrl, options = {}) {
        if (!(await this.ensureValidToken())) {
            throw new Error('Failed to refresh token');
        }

        const url = `${this.baseUrl}/api/analyze`;
        const headers = {
            'Authorization': `Bearer ${this.accessToken}`,
            'Content-Type': 'application/json'
        };

        const data = {
            media_type: mediaType,
            media_url: mediaUrl,
            save_to_db: options.saveToDb !== false
        };

        if (options.prompt) {
            data.prompt = options.prompt;
        }
        if (options.maxTokens) {
            data.max_tokens = options.maxTokens;
        }
        if (options.videoFrames && mediaType === 'video') {
            data.video_frames = options.videoFrames;
        }

        try {
            const response = await fetch(url, {
                method: 'POST',
                headers: headers,
                body: JSON.stringify(data)
            });

            return await response.json();
        } catch (error) {
            console.error('Analysis error:', error);
            throw error;
        }
    }

    async batchAnalyze(requests) {
        if (!(await this.ensureValidToken())) {
            throw new Error('Failed to refresh token');
        }

        const url = `${this.baseUrl}/api/batch_analyze`;
        const headers = {
            'Authorization': `Bearer ${this.accessToken}`,
            'Content-Type': 'application/json'
        };

        const data = {
            requests: requests
        };

        try {
            const response = await fetch(url, {
                method: 'POST',
                headers: headers,
                body: JSON.stringify(data)
            });

            return await response.json();
        } catch (error) {
            console.error('Batch analysis error:', error);
            throw error;
        }
    }

    async queryResults(queryType, params = {}) {
        if (!(await this.ensureValidToken())) {
            throw new Error('Failed to refresh token');
        }

        const url = `${this.baseUrl}/api/query`;
        const headers = {
            'Authorization': `Bearer ${this.accessToken}`,
            'Content-Type': 'application/json'
        };

        const data = {
            query_type: queryType,
            ...params
        };

        try {
            const response = await fetch(url, {
                method: 'POST',
                headers: headers,
                body: JSON.stringify(data)
            });

            return await response.json();
        } catch (error) {
            console.error('Query error:', error);
            throw error;
        }
    }

    async getStatus() {
        const url = `${this.baseUrl}/api/status`;

        try {
            const response = await fetch(url);
            return await response.json();
        } catch (error) {
            console.error('Status error:', error);
            throw error;
        }
    }
}


// 使用示例
(async () => {
    // 初始化客户端
    const client = new DoubaoAnalyzerClient(
        'http://localhost:8080',
        'admin',
        'admin123'
    );

    try {
        // 分析单张图片
        const result = await client.analyzeMedia(
            'image',
            'https://example.com/image.jpg',
            {
                prompt: '请详细描述这张图片的内容'
            }
        );
        console.log(JSON.stringify(result, null, 2));

        // 批量分析
        const requests = [
            {
                media_type: 'image',
                media_url: 'https://example.com/image1.jpg'
            },
            {
                media_type: 'video',
                media_url: 'https://example.com/video1.mp4',
                video_frames: 8
            }
        ];
        const batchResult = await client.batchAnalyze(requests);
        console.log(JSON.stringify(batchResult, null, 2));

        // 查询结果
        const queryResult = await client.queryResults(
            'tag',
            { tag: '风景' }
        );
        console.log(JSON.stringify(queryResult, null, 2));

    } catch (error) {
        console.error('Error:', error);
    }
})();
```

## 集成指南

### Web应用集成

1. **前端集成**:
   - 使用JavaScript SDK进行API调用
   - 实现令牌管理和自动刷新
   - 添加错误处理和重试机制

2. **后端集成**:
   - 在后端服务中集成API调用
   - 使用服务端令牌，避免在前端暴露认证信息
   - 实现API调用日志和监控

3. **安全考虑**:
   - 使用HTTPS保护API通信
   - 实现请求签名验证(可选)
   - 限制API访问频率

### 移动应用集成

1. **Android集成**:
   ```java
   // 示例代码：使用OkHttp和Retrofit进行API调用
   public class DoubaoAnalyzerService {
       private static final String BASE_URL = "http://your-server:8080/api";
       private String accessToken;
       private String refreshToken;

       // 登录获取令牌
       public boolean login(String username, String password) {
           // 实现登录逻辑
           return true;
       }

       // 刷新令牌
       public boolean refreshAccessToken() {
           // 实现令牌刷新逻辑
           return true;
       }

       // 分析图片
       public AnalysisResult analyzeImage(String imageUrl) {
           // 实现图片分析逻辑
           return null;
       }
   }
   ```

2. **iOS集成**:
   ```swift
   // 示例代码：使用URLSession进行API调用
   class DoubaoAnalyzerService {
       private let baseURL = "http://your-server:8080/api"
       private var accessToken: String?
       private var refreshToken: String?

       // 登录获取令牌
       func login(username: String, password: String, completion: @escaping (Bool) -> Void) {
           // 实现登录逻辑
       }

       // 刷新令牌
       func refreshAccessToken(completion: @escaping (Bool) -> Void) {
           // 实现令牌刷新逻辑
       }

       // 分析图片
       func analyzeImage(imageUrl: String, completion: @escaping (AnalysisResult?) -> Void) {
           // 实现图片分析逻辑
       }
   }
   ```

### 微服务集成

1. **服务间通信**:
   - 使用服务账号进行API认证
   - 实现服务发现和负载均衡
   - 添加熔断和降级机制

2. **消息队列集成**:
   - 将分析任务放入消息队列
   - 实现异步处理和结果通知
   - 支持任务优先级和重试

3. **数据同步**:
   - 实现分析结果的数据同步
   - 支持增量更新和全量同步
   - 处理数据冲突和一致性

### 企业系统集成

1. **SSO集成**:
   - 集成企业单点登录系统
   - 实现令牌映射和转换
   - 支持多种认证协议

2. **权限管理**:
   - 集成企业权限管理系统
   - 实现细粒度权限控制
   - 支持角色和策略管理

3. **审计日志**:
   - 记录API调用和分析结果
   - 支持日志检索和分析
   - 满足合规性要求

### 部署架构

1. **高可用部署**:
   ```
   +-----------------+       +-----------------+
   |   负载均衡器     |<----->|   API服务器集群   |
   +-----------------+       +-----------------+
           |                           |
           v                           v
   +-----------------+       +-----------------+
   |   数据库主节点   |<----->|   数据库从节点   |
   +-----------------+       +-----------------+
   ```

2. **容器化部署**:
   ```yaml
   # docker-compose.yml 示例
   version: '3'
   services:
     doubao-api:
       image: doubao/analyzer:latest
       ports:
         - "8080:8080"
       environment:
         - DOUBAO_API_KEY=${API_KEY}
         - DB_HOST=mysql
         - DB_USER=doubao_user
         - DB_PASSWORD=${DB_PASSWORD}
         - DB_NAME=doubao_analyzer
       depends_on:
         - mysql

     mysql:
       image: mysql:5.7
       environment:
         - MYSQL_ROOT_PASSWORD=${ROOT_PASSWORD}
         - MYSQL_DATABASE=doubao_analyzer
         - MYSQL_USER=doubao_user
         - MYSQL_PASSWORD=${DB_PASSWORD}
       volumes:
         - mysql_data:/var/lib/mysql

   volumes:
     mysql_data:
   ```

3. **Kubernetes部署**:
   ```yaml
   # k8s-deployment.yaml 示例
   apiVersion: apps/v1
   kind: Deployment
   metadata:
     name: doubao-analyzer
   spec:
     replicas: 3
     selector:
       matchLabels:
         app: doubao-analyzer
     template:
       metadata:
         labels:
           app: doubao-analyzer
       spec:
         containers:
         - name: doubao-analyzer
           image: doubao/analyzer:latest
           ports:
           - containerPort: 8080
           env:
           - name: DOUBAO_API_KEY
             valueFrom:
               secretKeyRef:
                 name: doubao-secrets
                 key: api-key
           - name: DB_HOST
             value: mysql-service
           - name: DB_USER
             value: doubao_user
           - name: DB_PASSWORD
             valueFrom:
               secretKeyRef:
                 name: doubao-secrets
                 key: db-password
           - name: DB_NAME
             value: doubao_analyzer
   ---
   apiVersion: v1
   kind: Service
   metadata:
     name: doubao-analyzer-service
   spec:
     selector:
       app: doubao-analyzer
     ports:
     - protocol: TCP
       port: 80
       targetPort: 8080
     type: LoadBalancer
   ```
