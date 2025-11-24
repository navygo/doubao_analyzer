#!/bin/bash

# Ollama优化启动脚本 - 针对T4显卡(15G)及视觉模型优化

# 设置环境变量
export OLLAMA_HOST=0.0.0.0:11434
export OLLAMA_NUM_PARALLEL=2           # 降低并行数，T4算力有限，避免排队恶化
export OLLAMA_MAX_LOADED_MODELS=1      # 保持当前值，显存有限
export OLLAMA_MAX_QUEUE=256            # 进一步减少队列大小，匹配降低的并行度
export OLLAMA_KEEP_ALIVE=30m           # 增加模型保持时间，减少重载
export OLLAMA_LOAD_TIMEOUT=5m          # 减少加载超时，更快失败反馈
export OLLAMA_FLASH_ATTENTION=false    # T4显卡关闭Flash Attention以避免兼容性问题
export OLLAMA_CONTEXT_LENGTH=4096      # 适度增加上下文长度以适应视觉模型对长上下文的需求[citation:2]
export OLLAMA_GPU_OVERHEAD=0           # 减少GPU开销
export OLLAMA_DEBUG=INFO               # 保持INFO级别日志
export OLLAMA_GPU_LAYERS=35            # 设置为一个合理的层数（如35），而非999，根据实际模型调整[citation:10]
export OLLAMA_BATCH_SIZE=256           # 使用适中的批处理大小，平衡速度和显存[citation:2]
export OLLAMA_SCHED_SPREAD=true        # 在多个GPU上分散请求（如果有多个GPU）

# 预加载视觉模型
echo "预加载视觉模型..."
# /usr/local/bin/ollama pull minicpm-v:latest &  # 建议使用模型

# 启动Ollama
echo "启动优化后的Ollama服务..."
echo "配置参数（针对T4 15G）:"
echo "- 并行处理数: $OLLAMA_NUM_PARALLEL"
echo "- 最大队列: $OLLAMA_MAX_QUEUE"
echo "- Flash Attention: $OLLAMA_FLASH_ATTENTION"
echo "- 上下文长度: $OLLAMA_CONTEXT_LENGTH"
echo "- GPU层数: $OLLAMA_GPU_LAYERS"
echo "- 批处理大小: $OLLAMA_BATCH_SIZE"
ollama serve