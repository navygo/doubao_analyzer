#!/bin/bash

# Ollama T4 GPU专用优化脚本，针对qwen3-vl:8b模型

# 设置环境变量
export OLLAMA_HOST=0.0.0.0:11434
export OLLAMA_NUM_PARALLEL=4          # 并行处理数量
export OLLAMA_MAX_LOADED_MODELS=1     # T4显存有限，保持1
export OLLAMA_MAX_QUEUE=1024          # 增加队列大小
export OLLAMA_KEEP_ALIVE=15m          # 进一步增加模型保持时间
export OLLAMA_LOAD_TIMEOUT=10m        # 载入超时
export OLLAMA_FLASH_ATTENTION=true     # 启用Flash Attention
export OLLAMA_CONTEXT_LENGTH=2048      # 降低上下文长度
export OLLAMA_GPU_OVERHEAD=0           # 减少GPU开销
export OLLAMA_DEBUG=INFO              # 保持INFO级别日志

# T4专用优化
export OLLAMA_SCHED_SPREAD=true       # 更均匀地调度任务
export OLLAMA_KV_CACHE_TYPE=f16        # 使用半精度KV缓存，节省显存
export OLLAMA_LLM_LIBRARY=cuda         # 明确使用CUDA库

# 显存优化 - 针对T4的15GB显存
export OLLAMA_VRAM_THRESHOLD=15        # 设置显存阈值为15GB，避免进入低显存模式

# 启动Ollama
echo "启动T4 GPU优化后的Ollama服务..."
echo "配置参数:"
echo "- 并行处理数: $OLLAMA_NUM_PARALLEL"
echo "- 最大队列: $OLLAMA_MAX_QUEUE"
echo "- Flash Attention: $OLLAMA_FLASH_ATTENTION"
echo "- 上下文长度: $OLLAMA_CONTEXT_LENGTH"
echo "- KV缓存类型: $OLLAMA_KV_CACHE_TYPE"
echo "- 显存阈值: $OLLAMA_VRAM_THRESHOLD GB"

ollama serve
