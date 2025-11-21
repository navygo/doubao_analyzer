#!/bin/bash

# Ollama优化启动脚本，针对T4 GPU和qwen3-vl:8b模型优化

# 设置环境变量
export OLLAMA_HOST=0.0.0.0:11434
export OLLAMA_NUM_PARALLEL=4          # 增加并行处理数量
export OLLAMA_MAX_LOADED_MODELS=1     # 保持当前值，T4显存有限
export OLLAMA_MAX_QUEUE=1024          # 增加队列大小
export OLLAMA_KEEP_ALIVE=10m          # 增加模型保持时间，减少重载
export OLLAMA_LOAD_TIMEOUT=10m        # 增加载入超时
export OLLAMA_FLASH_ATTENTION=true     # 启用Flash Attention
export OLLAMA_CONTEXT_LENGTH=2048      # 降低上下文长度，适合图像分析
export OLLAMA_GPU_OVERHEAD=0           # 减少GPU开销
export OLLAMA_DEBUG=INFO              # 保持INFO级别日志

# 启动Ollama
echo "启动优化后的Ollama服务..."
echo "配置参数:"
echo "- 并行处理数: $OLLAMA_NUM_PARALLEL"
echo "- 最大队列: $OLLAMA_MAX_QUEUE"
echo "- Flash Attention: $OLLAMA_FLASH_ATTENTION"
echo "- 上下文长度: $OLLAMA_CONTEXT_LENGTH"

ollama serve
