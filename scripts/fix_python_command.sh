#!/bin/bash

# 修复Python命令中的语法错误

cd /home/whj00/doubao_analyzer/src

# 备份原文件
cp ExcelProcessor.cpp ExcelProcessor.cpp.bak

# 修复Python命令中的语法错误
sed -i 's/python3 -c \\"/python3 -c \'/g' ExcelProcessor.cpp
sed -i 's/\\"/'"'"'/g' ExcelProcessor.cpp
sed -i 's/f'\''导入错误: {e}'"\''/f'\''导入错误: {}'"'"'/g' ExcelProcessor.cpp
sed -i 's/f'\''转换错误: {e}'"\''/f'\''转换错误: {}'"'"'/g' ExcelProcessor.cpp

# 修复CSV到XLSX的转换问题
sed -i 's/python3 -c \"/python3 -c \'/g' ExcelProcessor.cpp

echo "Python命令已修复"
