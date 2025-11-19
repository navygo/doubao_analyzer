#!/bin/bash

# 安装Python依赖脚本
echo "正在安装Python依赖..."

# 安装pandas和openpyxl
pip3 install pandas openpyxl

# 检查安装是否成功
python3 -c "import pandas, openpyxl; print('依赖安装成功')" 2>/dev/null

if [ $? -eq 0 ]; then
    echo "Python依赖安装成功"
else
    echo "Python依赖安装失败，请检查错误信息"
    exit 1
fi
