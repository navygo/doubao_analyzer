#!/bin/bash

# Ollama优化启动脚本，针对图片分析和视觉模型优化

# 设置环境变量
export OLLAMA_HOST=0.0.0.0:11434
export OLLAMA_NUM_PARALLEL=4          # 适中的并行处理数量，避免资源竞争
export OLLAMA_MAX_LOADED_MODELS=1     # 保持当前值，显存有限
export OLLAMA_MAX_QUEUE=512           # 减少队列大小，加快响应
export OLLAMA_KEEP_ALIVE=30m          # 增加模型保持时间，减少重载
export OLLAMA_LOAD_TIMEOUT=5m         # 减少加载超时，更快失败反馈
export OLLAMA_FLASH_ATTENTION=true     # 启用Flash Attention
export OLLAMA_CONTEXT_LENGTH=2048      # 减少上下文长度，加快图片处理
export OLLAMA_GPU_OVERHEAD=0           # 减少GPU开销
export OLLAMA_DEBUG=INFO              # 保持INFO级别日志
export OLLAMA_GPU_LAYERS=999          # 尽可能多地将层加载到GPU
export OLLAMA_BATCH_SIZE=512          # 增加批处理大小，提高吞吐量
export OLLAMA_SCHED_SPREAD=true       # 在多个GPU上分散请求（如果有多个GPU）

# 预加载视觉模型
echo "预加载视觉模型..."
/usr/local/bin/ollama pull qwen3-vl:2b &

# 启动Ollama
echo "启动优化后的Ollama服务..."
echo "配置参数:"
echo "- 并行处理数: $OLLAMA_NUM_PARALLEL"
echo "- 最大队列: $OLLAMA_MAX_QUEUE"
echo "- Flash Attention: $OLLAMA_FLASH_ATTENTION"
echo "- 上下文长度: $OLLAMA_CONTEXT_LENGTH"
echo "- GPU层数: $OLLAMA_GPU_LAYERS"
echo "- 批处理大小: $OLLAMA_BATCH_SIZE"

ollama serve
