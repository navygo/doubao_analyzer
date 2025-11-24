#!/bin/bash

# Ollama优化启动脚本 - 针对双T4显卡(15Gx2)精细调优

# 设置环境变量
export OLLAMA_HOST=0.0.0.0:11434
export OLLAMA_NUM_PARALLEL=6           # 适度降低并行数，避免资源竞争
export OLLAMA_MAX_LOADED_MODELS=2      # 降低加载模型数，每卡一个更稳定
export OLLAMA_MAX_QUEUE=192            # 适度队列大小
export OLLAMA_KEEP_ALIVE=20m           # 增加保持时间，减少重载开销
export OLLAMA_LOAD_TIMEOUT=8m          # 增加加载超时，适应大模型
export OLLAMA_FLASH_ATTENTION=false    # T4保持关闭
export OLLAMA_CONTEXT_LENGTH=16384     # 进一步增加上下文长度[citation:2]
export OLLAMA_GPU_OVERHEAD=2048        # 增加GPU开销预算
export OLLAMA_DEBUG=INFO
export OLLAMA_SCHED_SPREAD=true        # 多GPU负载均衡
export OLLAMA_GPU_LAYERS=99
export OLLAMA_BATCH_SIZE=256           # 降低批处理大小，避免OOM[citation:2]
export OLLAMA_NUM_THREADS=10           # 适度CPU线程

# 多GPU配置
export OLLAMA_GPUS=2
export CUDA_VISIBLE_DEVICES=0,1

# 内存和性能优化
export OLLAMA_MMAP=true
export OLLAMA_NUM_CPU_THREADS=10
export OLLAMA_LAYERS_GPU=99            # 明确指定GPU层数

# 针对T4的特定优化
export OLLAMA_GRAPH_OPTIMIZATION_LEVEL=3
export OLLAMA_ENABLE_CPU_FALLBACK=true  # 启用CPU回退

echo "服务器配置检测:"
echo "- GPU: 2 x Tesla T4 15GB (已识别)"
echo "- 内存: 64GB"
echo "- CPU: 16核"
echo "- 可用显存: CUDA0: 14.6GiB, CUDA1: 14.5GiB"

# 检查GPU状态
echo "检查GPU状态..."
nvidia-smi --query-gpu=index,name,memory.total,memory.free --format=csv

# 预加载常用模型函数
preload_models() {
    echo "开始预加载模型..."

    # 视觉模型
    # /usr/local/bin/ollama pull llava:13b &

    # 通用模型
    # /usr/local/bin/ollama pull qwen2.5:7b &

    # 等待所有预加载完成
    wait
    echo "模型预加载完成"
}

# 可选：取消注释以下行来启用模型预加载
# preload_models &

# 启动Ollama
echo "启动优化后的Ollama服务（双T4精细调优）..."
echo "配置参数:"
echo "- 并行处理数: $OLLAMA_NUM_PARALLEL"
echo "- 最大队列: $OLLAMA_MAX_QUEUE"
echo "- GPU数量: $OLLAMA_GPUS"
echo "- 上下文长度: $OLLAMA_CONTEXT_LENGTH"
echo "- 批处理大小: $OLLAMA_BATCH_SIZE"
echo "- CPU线程数: $OLLAMA_NUM_THREADS"
echo "- 多GPU负载均衡: $OLLAMA_SCHED_SPREAD"
echo "- 内存映射: $OLLAMA_MMAP"
echo "- GPU层数: $OLLAMA_GPU_LAYERS"

# 设置ulimit以提高性能
ulimit -n 65536

ollama serve