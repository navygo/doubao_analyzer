# Excel文件处理功能

## 概述

本系统现在支持从Excel文件（.xlsx和.csv格式）读取媒体URL，并批量分析这些媒体文件。

## 功能特点

1. 支持.xlsx和.csv格式的Excel文件
2. 自动识别中文内容类型（图片、视频等）
3. 支持UTF-8编码，正确处理中文内容
4. 提供多种XLSX文件处理方案，确保系统可靠性

## 安装依赖

在使用Excel处理功能前，需要安装以下Python依赖：

```bash
# 运行安装脚本
chmod +x scripts/install_python_deps.sh
./scripts/install_python_deps.sh
```

或者手动安装：

```bash
pip3 install pandas openpyxl
```

## 使用方法

### API接口

通过API接口使用Excel分析功能：

```json
{
  "excel_path": "path/to/your/file.xlsx",
  "output_path": "path/to/output/file.xlsx",
  "prompt": "分析提示词",
  "max_tokens": 1500,
  "save_to_db": true
}
```

### Excel文件格式

支持的Excel文件格式：

```
序号,内容类型,内容发布位置,内容id,内容地址,标签
1,图片,国内,12511181716,https://example.com/image1.jpg,
2,视频,海外,12511181717,https://example.com/video1.mp4,
```

## 处理流程

1. 系统检测文件扩展名（.xlsx或.csv）
2. 对于.xlsx文件，尝试以下方案（按优先级）：
   - 方案1：使用Python pandas库转换为CSV
   - 方案2：使用LibreOffice转换为CSV
   - 方案3：使用内置解析器直接解析XLSX文件内容
3. 读取CSV文件内容
4. 解析每行数据，创建分析任务
5. 批量处理媒体文件
6. 将分析结果写回Excel文件

## 故障排除

### 常见问题

1. **"ModuleNotFoundError: No module named 'openpyxl'"**
   - 解决方案：运行安装脚本 `./scripts/install_python_deps.sh`
   - 或手动安装：`pip3 install pandas openpyxl`

2. **"无法转换XLSX文件"**
   - 确保已安装LibreOffice：`sudo apt-get install libreoffice`
   - 检查文件权限和路径

3. **"分析处理异常"**
   - 检查媒体URL是否有效
   - 检查网络连接
   - 查看详细错误日志

## 技术细节

### XLSX文件处理

XLSX文件实际上是ZIP压缩包，包含多个XML文件。系统提供三种处理方案：

1. **Python pandas方案**：
   - 使用pandas.read_excel()读取文件
   - 使用pandas.to_csv()输出为CSV格式
   - 需要pandas和openpyxl库

2. **LibreOffice方案**：
   - 使用命令行工具转换文件
   - 无需Python依赖
   - 需要安装LibreOffice

3. **内置解析器方案**：
   - 直接解析XLSX文件结构
   - 提取xl/worksheets/sheet1.xml文件
   - 无需外部依赖
   - 仅支持基本单元格内容

### 中文支持

系统全面支持中文内容：

1. 文件路径支持中文
2. 内容类型支持中文识别（图片、视频、照片、影片等）
3. 输出文件保留UTF-8 BOM标记
4. 错误消息支持中文显示
