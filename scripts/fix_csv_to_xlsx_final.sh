#!/bin/bash

# 最终修复CSV到XLSX转换问题

cd /home/whj00/doubao_analyzer/src

# 备份原文件
cp ExcelProcessor.cpp ExcelProcessor.cpp.bak3

# 修改转换命令
sed -i '489,494c        std::string command = "python3 csv_cleaner.py " + temp_csv_path + " " + output_file_path;' ExcelProcessor.cpp

echo "CSV到XLSX转换已修复"
