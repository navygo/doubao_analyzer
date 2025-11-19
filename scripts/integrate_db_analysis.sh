#!/bin/bash

# 集成数据库分析功能到ApiServer.cpp

cd /home/whj00/doubao_analyzer/src

# 备份原文件
cp ApiServer.cpp ApiServer.cpp.bak

# 在ApiServer.cpp末尾添加DbMediaAnalysis.cpp的内容
cat DbMediaAnalysis.cpp >> ApiServer.cpp

# 删除临时文件
rm DbMediaAnalysis.cpp

echo "数据库分析功能已集成到ApiServer.cpp"
