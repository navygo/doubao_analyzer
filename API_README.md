# 豆包媒体分析API服务器

豆包媒体分析工具的API服务器版本，提供HTTP接口供三方系统调用，实现图片和视频的智能分析功能。

## 功能特性

- 🖼️ **图片分析**: 通过URL分析图片内容
- 🎬 **视频分析**: 通过URL分析视频内容
- 📁 **标签提取**: 自动从分析结果中提取标签
- 💾 **数据库存储**: 支持将分析结果存储到MySQL数据库
- 🌐 **RESTful API**: 提供简单的HTTP接口

## 安装与编译

### 1. 编译项目
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 2. 安装到系统
```bash
sudo make install
```

## 使用方法

### 启动API服务器
```bash
# 基本用法
doubao_api_server --api-key YOUR_API_KEY

# 自定义端口和主机
doubao_api_server --api-key YOUR_API_KEY --port 8080 --host 0.0.0.0

# 查看帮助
doubao_api_server --help
```

### API接口

#### 图片分析接口

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

#### 视频分析接口

**请求示例:**
```json
{
    "media_type": "video",
    "media_url": "https://example.com/video.mp4",
    "prompt": "请分析这个视频的主要内容",
    "max_tokens": 2000,
    "video_frames": 8,
    "save_to_db": true
}
```

**响应示例:**
```json
{
    "success": true,
    "message": "视频分析成功",
    "response_time": 5.7,
    "data": {
        "content": "这个视频展示了一个人在山间徒步的过程...",
        "tags": ["徒步", "山间", "户外"],
        "response_time": 5.5,
        "usage": {
            "prompt_tokens": 150,
            "completion_tokens": 300,
            "total_tokens": 450
        },
        "saved_to_db": true
    }
}
```

### 请求参数说明

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| media_type | string | 是 | 媒体类型，"image"或"video" |
| media_url | string | 是 | 图片或视频的URL地址 |
| prompt | string | 否 | 自定义提示词，留空则使用默认提示词 |
| max_tokens | int | 否 | 最大令牌数，图片默认1500，视频默认2000 |
| video_frames | int | 否 | 视频提取帧数，仅视频分析有效，默认为5 |
| save_to_db | bool | 否 | 是否将结果保存到数据库，默认为true |

### 错误响应示例

```json
{
    "success": false,
    "message": "图片下载失败: https://example.com/image.jpg",
    "error": "Image download failed",
    "response_time": 0.5
}
```

## 数据库配置

API服务器使用与命令行工具相同的数据库配置。请确保已正确配置MySQL数据库，详见主README文档中的数据库配置部分。

## 注意事项

1. 服务器会自动下载URL指定的媒体文件到临时目录，分析完成后自动删除
2. 下载超时设置为5分钟，大文件可能需要更长时间
3. 确保服务器有足够的临时存储空间
4. API服务器默认监听8080端口，可通过--port参数修改
5. 默认绑定到0.0.0.0，允许外部访问，生产环境建议使用防火墙限制访问

## 故障排除

- **连接失败**: 检查API密钥是否正确，网络是否通畅
- **下载失败**: 确认URL是否可访问，文件是否过大
- **数据库错误**: 检查数据库配置是否正确，MySQL服务是否运行
- **端口占用**: 使用--port参数指定其他端口

## 许可证

MIT License
