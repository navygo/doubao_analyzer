#!/bin/bash

# 一键启动优化后的Ollama和doubao_analyzer
# 适用于T4 GPU和qwen3-vl:8b模型

# 检查脚本执行权限
if [ ! -x "$0" ]; then
    chmod +x "$0"
fi

# 设置颜色输出
RED='[0;31m'
GREEN='[0;32m'
YELLOW='[1;33m'
NC='[0m' # No Color

echo -e "${GREEN}=== T4 GPU优化启动脚本 ===${NC}"
echo -e "${YELLOW}正在启动优化后的Ollama服务...${NC}"

# 启动Ollama服务（后台运行）
nohup /home/whj00/doubao_analyzer/scripts/ollama_t4_optimized.sh > /tmp/ollama.log 2>&1 &
OLLAMA_PID=$!

# 等待Ollama服务启动
echo -e "${YELLOW}等待Ollama服务启动...${NC}"
sleep 10

# 检查Ollama是否成功启动
if curl -s http://127.0.0.1:11434/api/tags > /dev/null; then
    echo -e "${GREEN}✓ Ollama服务已成功启动 (PID: $OLLAMA_PID)${NC}"
else
    echo -e "${RED}✗ Ollama服务启动失败，请检查日志: /tmp/ollama.log${NC}"
    exit 1
fi

# 检查模型是否已下载
echo -e "${YELLOW}检查qwen3-vl:8b模型...${NC}"
if ! curl -s http://127.0.0.1:11434/api/tags | grep -q "qwen3-vl:8b"; then
    echo -e "${YELLOW}模型未找到，正在下载qwen3-vl:8b...${NC}"
    ollama pull qwen3-vl:8b
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ 模型下载失败${NC}"
        kill $OLLAMA_PID
        exit 1
    fi
    echo -e "${GREEN}✓ 模型下载完成${NC}"
else
    echo -e "${GREEN}✓ qwen3-vl:8b模型已就绪${NC}"
fi

# 启动应用程序
echo -e "${YELLOW}启动优化后的doubao_analyzer...${NC}"
/home/whj00/doubao_analyzer/scripts/app_t4_optimized.sh "$@"
APP_EXIT_CODE=$?

# 清理
echo -e "${YELLOW}正在停止Ollama服务...${NC}"
kill $OLLAMA_PID

# 根据应用程序退出码返回
if [ $APP_EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✓ 应用程序正常退出${NC}"
else
    echo -e "${RED}✗ 应用程序异常退出 (代码: $APP_EXIT_CODE)${NC}"
fi

exit $APP_EXIT_CODE
