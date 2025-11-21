#!/bin/bash

# 应用程序T4 GPU优化脚本
# 配置doubao_analyzer以使用优化后的Ollama

# 设置环境变量
export DOUBAO_BASE_URL="http://127.0.0.1:11434/api/chat"  # 使用chat端点而非generate
export DOUBAO_MODEL_NAME="qwen3-vl:8b"
export DOUBAO_MAX_TOKENS=1000         # 减少最大令牌数，提高响应速度
export DOUBAO_TEMPERATURE=0.1         # 降低温度，提高确定性
export DOUBAO_BATCH_SIZE=4           # 批处理大小，匹配OLLAMA_NUM_PARALLEL
export DOUBAO_MAX_CONCURRENT=4        # 最大并发任务数

# 图像处理优化
export DOUBAO_IMAGE_MAX_SIZE=768      # 降低图像最大尺寸
export DOUBAO_IMAGE_QUALITY=75        # 降低JPEG质量
export DOUBAO_IMAGE_MAX_FILESIZE=200000  # 200KB

# 视频处理优化
export DOUBAO_VIDEO_FRAMES=3          # 减少视频帧数
export DOUBAO_VIDEO_MAX_SIZE=768      # 降低视频帧尺寸

# 性能优化
export DOUBAO_ENABLE_GPU=true          # 启用GPU加速
export DOUBAO_CACHE_ENABLED=true      # 启用结果缓存
export DOUBAO_CACHE_TTL=3600          # 缓存1小时

# 启动应用程序
echo "启动T4 GPU优化后的doubao_analyzer..."
echo "配置参数:"
echo "- API端点: $DOUBAO_BASE_URL"
echo "- 模型: $DOUBAO_MODEL_NAME"
echo "- 最大令牌数: $DOUBAO_MAX_TOKENS"
echo "- 批处理大小: $DOUBAO_BATCH_SIZE"
echo "- 图像最大尺寸: ${DOUBAO_IMAGE_MAX_SIZE}px"
echo "- 视频帧数: $DOUBAO_VIDEO_FRAMES"

# 运行应用程序
./doubao_analyzer --base-url "$DOUBAO_BASE_URL" --model "$DOUBAO_MODEL_NAME"                   --max-tokens "$DOUBAO_MAX_TOKENS" --temperature "$DOUBAO_TEMPERATURE"                   --batch-size "$DOUBAO_BATCH_SIZE" --max-concurrent "$DOUBAO_MAX_CONCURRENT"                   --image-max-size "$DOUBAO_IMAGE_MAX_SIZE" --image-quality "$DOUBAO_IMAGE_QUALITY"                   --video-frames "$DOUBAO_VIDEO_FRAMES" --video-max-size "$DOUBAO_VIDEO_MAX_SIZE"                   --enable-gpu "$DOUBAO_ENABLE_GPU" --cache-enabled "$DOUBAO_CACHE_ENABLED"                   --cache-ttl "$DOUBAO_CACHE_TTL" "$@"
