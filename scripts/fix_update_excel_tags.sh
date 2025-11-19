#!/bin/bash

# 替换update_excel_tags函数的脚本

cd /home/whj00/doubao_analyzer/src

# 备份原文件
cp ExcelProcessor.cpp ExcelProcessor.cpp.bak

# 提取update_excel_tags函数之前的代码
head -n 335 ExcelProcessor.cpp > temp1.cpp

# 添加新的update_excel_tags函数
cat ../src/update_excel_tags.cpp >> temp1.cpp

# 提取update_excel_tags函数之后的代码
tail -n +418 ExcelProcessor.cpp > temp2.cpp

# 合并文件
cat temp1.cpp temp2.cpp > ExcelProcessor.cpp

# 清理临时文件
rm temp1.cpp temp2.cpp

echo "update_excel_tags函数已更新"
