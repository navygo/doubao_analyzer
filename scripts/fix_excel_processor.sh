#!/bin/bash

# 修复ExcelProcessor.cpp中的问题

cd /home/whj00/doubao_analyzer/src

# 备份原文件
cp ExcelProcessor.cpp ExcelProcessor.cpp.bak

# 创建修复版本
cat > ExcelProcessor_fixed.cpp << 'EOF'
#include "ExcelProcessor.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <map>

// 解析CSV格式的一行
ExcelRowData ExcelProcessor::parse_csv_line(const std::string &line)
{
    ExcelRowData data;
    std::vector<std::string> cells;

    // 更健壮的CSV解析，支持引号内的逗号
    std::string cell;
    bool in_quotes = false;

    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];

        if (c == '"') {
            // 处理引号
            if (in_quotes && i + 1 < line.length() && line[i + 1] == '"') {
                // 双引号转义
                cell += '"';
                i++; // 跳过下一个引号
            } else {
                // 切换引号状态
                in_quotes = !in_quotes;
            }
        } else if (c == ',' && !in_quotes) {
            // 字段分隔符
            cells.push_back(cell);
            cell.clear();
        } else {
            cell += c;
        }
    }

    // 添加最后一个字段
    cells.push_back(cell);

    // 去除每个字段的前后空格和引号
    for (auto &field : cells) {
        // 去除前后空格
        field.erase(0, field.find_first_not_of(" 	"));
        field.erase(field.find_last_not_of(" 	") + 1);

        // 去除引号
        if (field.length() >= 2 && field.front() == '"' && field.back() == '"') {
            field = field.substr(1, field.length() - 2);
        }
    }

    // 根据Excel格式映射字段
    // 序号 内容类型 内容发布位置 内容id 内容地址 标签
    if (cells.size() >= 5) {
        data.sequence = cells[0];
        data.content_type = cells[1];
        data.location = cells[2];
        data.content_id = cells[3];
        data.url = cells[4];
        if (cells.size() >= 6) {
            data.tags = cells[5];
        }
    }

    return data;
}

// 从CSV文件读取数据
std::vector<ExcelRowData> ExcelProcessor::read_csv_file(const std::string &file_path)
{
    std::vector<ExcelRowData> data;

    // 使用UTF-8编码打开文件
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "无法打开CSV文件: " << file_path << std::endl;
        return data;
    }

    // 读取文件内容到字符串
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // 检查BOM标记并移除
    if (content.size() >= 3 && 
        (unsigned char)content[0] == 0xEF && 
        (unsigned char)content[1] == 0xBB && 
        (unsigned char)content[2] == 0xBF) {
        content = content.substr(3); // 移除UTF-8 BOM
    }

    std::stringstream ss(content);
    std::string line;
    bool first_line = true; // 跳过标题行

    while (std::getline(ss, line)) {
        if (first_line) {
            first_line = false;
            continue;
        }

        if (line.empty()) {
            continue;
        }

        ExcelRowData row_data = parse_csv_line(line);
        if (!row_data.url.empty()) {
            data.push_back(row_data);
        }
    }

    std::cout << "成功读取CSV文件，共 " << data.size() << " 行数据" << std::endl;

    return data;
}

// 从Excel文件(CSV或XLSX格式)读取数据
std::vector<ExcelRowData> ExcelProcessor::read_excel_file(const std::string &file_path)
{
    // 检查文件扩展名，确定文件类型
    std::string extension = std::filesystem::path(file_path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == ".xlsx") {
        return read_xlsx_file(file_path);
    } else {
        return read_csv_file(file_path);
    }
}

// 从XLSX文件读取数据
std::vector<ExcelRowData> ExcelProcessor::read_xlsx_file(const std::string &file_path)
{
    std::vector<ExcelRowData> data;

    std::cout << "正在处理XLSX文件: " << file_path << std::endl;

    // 方案1：使用系统命令将xlsx转换为csv
    std::string temp_csv_path = file_path + ".temp.csv";
    std::string command = "python3 -c '"
        "import sys; "
        "try: "
        "  import pandas as pd; "
        "  df = pd.read_excel('" + file_path + "'); "
        "  df.to_csv('" + temp_csv_path + "', index=False, encoding='utf-8-sig'); "
        "  print('转换成功'); "
        "except ImportError as e: "
        "  print('导入错误: {}'.format(e)); "
        "  sys.exit(1); "
        "except Exception as e: "
        "  print('转换错误: {}'.format(e)); "
        "  sys.exit(2)'";

    std::cout << "执行转换命令: " << command << std::endl;
    int result = system(command.c_str());

    if (result != 0) {
        std::cerr << "XLSX转CSV失败，尝试使用libreoffice..." << std::endl;

        // 方案2：使用libreoffice
        command = "libreoffice --headless --convert-to csv --outdir " + 
                  std::filesystem::path(file_path).parent_path().string() + " " + file_path;
        std::cout << "执行备用转换命令: " << command << std::endl;
        result = system(command.c_str());

        if (result != 0) {
            std::cerr << "LibreOffice转换失败，尝试使用内置简单解析器..." << std::endl;

            // 方案3：使用内置简单解析器（仅处理第一个工作表）
            // 创建临时CSV文件
            std::ofstream temp_file(temp_csv_path);
            if (temp_file.is_open()) {
                // 写入UTF-8 BOM
                temp_file.write("\xEF\xBB\xBF", 3);

                // 写入标题行
                temp_file << "序号,内容类型,内容发布位置,内容id,内容地址,标签\n";

                // 使用unzip命令提取xl/worksheets/sheet1.xml
                std::string xml_path = file_path + ".extract.xml";
                command = "unzip -p '" + file_path + "' xl/worksheets/sheet1.xml > '" + xml_path + "'";
                std::cout << "提取XML内容: " << command << std::endl;
                result = system(command.c_str());

                if (result == 0) {
                    // 读取XML文件并解析内容
                    std::ifstream xml_file(xml_path);
                    if (xml_file.is_open()) {
                        std::string line;
                        bool in_cell = false;
                        std::string cell_value;
                        int row_count = 0;
                        int col_count = 0;
                        std::vector<std::string> row_values(6, ""); // 6列

                        while (std::getline(xml_file, line)) {
                            // 查找单元格开始标记
                            if (line.find("<c r=") != std::string::npos) {
                                in_cell = true;
                                col_count++;
                                continue;
                            }

                            // 查找单元格结束标记
                            if (in_cell && line.find("</c>") != std::string::npos) {
                                in_cell = false;

                                // 查找值标记
                                size_t v_pos = line.find("<v>");
                                if (v_pos != std::string::npos) {
                                    size_t end_v = line.find("</v>", v_pos);
                                    if (end_v != std::string::npos) {
                                        cell_value = line.substr(v_pos + 3, end_v - v_pos - 3);
                                    }
                                }

                                // 根据列数存储值
                                if (col_count <= 6) {
                                    row_values[col_count % 6] = cell_value;
                                }

                                // 如果是第6列（标签列）或行结束，创建行数据
                                if (col_count % 6 == 0 || line.find("</row>") != std::string::npos) {
                                    if (row_count > 0) { // 跳过标题行
                                        ExcelRowData row_data;
                                        row_data.sequence = row_values[0];
                                        row_data.content_type = row_values[1];
                                        row_data.location = row_values[2];
                                        row_data.content_id = row_values[3];
                                        row_data.url = row_values[4];
                                        row_data.tags = row_values[5];

                                        if (!row_data.url.empty()) {
                                            data.push_back(row_data);
                                        }
                                    }

                                    // 重置行值
                                    for (auto &val : row_values) {
                                        val.clear();
                                    }
                                    row_count++;
                                }

                                cell_value.clear();
                            }
                        }
                        xml_file.close();
                    }

                    // 删除临时XML文件
                    std::filesystem::remove(xml_path);
                }

                temp_file.close();
            }

            if (data.empty()) {
                std::cerr << "无法解析XLSX文件，请确保文件格式正确" << std::endl;
                return data;
            }
        } else {
            // libreoffice输出的文件名可能不同，需要查找
            std::string base_name = std::filesystem::path(file_path).stem().string();
            temp_csv_path = std::filesystem::path(file_path).parent_path().string() + "/" + base_name + ".csv";
        }
    }

    // 读取转换后的CSV文件
    data = read_csv_file(temp_csv_path);

    // 删除临时CSV文件
    std::filesystem::remove(temp_csv_path);

    return data;
}

// 将Excel数据转换为分析任务
std::vector<AnalysisTask> ExcelProcessor::create_analysis_tasks(
    const std::vector<ExcelRowData> &excel_data,
    const std::string &prompt,
    int max_tokens,
    bool save_to_db)
{
    std::vector<AnalysisTask> tasks;

    for (const auto &row : excel_data) {
        // 跳过没有URL的行
        if (row.url.empty()) {
            continue;
        }

        AnalysisTask task;
        task.id = "task_" + row.content_id;
        task.media_url = row.url;
        task.prompt = prompt;
        task.max_tokens = max_tokens;
        task.save_to_db = save_to_db;
        task.file_id = row.content_id; // 使用内容ID作为file_id

        // 根据内容类型确定媒体类型
        // 转换为小写进行比较
        std::string content_type_lower = row.content_type;
        std::transform(content_type_lower.begin(), content_type_lower.end(), content_type_lower.begin(), ::tolower);

        if (content_type_lower == "图片" || content_type_lower == "image" || content_type_lower == "照片") {
            task.media_type = "image";
        } else if (content_type_lower == "视频" || content_type_lower == "video" || content_type_lower == "影片") {
            task.media_type = "video";
        } else {
            // 默认根据URL扩展名判断
            std::string extension = std::filesystem::path(row.url).extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if (extension == ".jpg" || extension == ".jpeg" || extension == ".png" || 
                extension == ".gif" || extension == ".bmp" || extension == ".webp") {
                task.media_type = "image";
            } else {
                task.media_type = "video";
            }
        }

        tasks.push_back(task);
    }

    std::cout << "成功创建 " << tasks.size() << " 个分析任务" << std::endl;

    return tasks;
}

// 更新Excel文件中的标签列
bool ExcelProcessor::update_excel_tags(
    const std::string &input_file_path,
    const std::string &output_file_path,
    const std::vector<std::pair<std::string, std::string>> &file_id_tags)
{
    // 创建file_id到标签的映射
    std::map<std::string, std::string> tag_map;
    for (const auto &pair : file_id_tags) {
        tag_map[pair.first] = pair.second;
    }

    // 检查文件扩展名，确定文件类型
    std::string input_extension = std::filesystem::path(input_file_path).extension().string();
    std::string output_extension = std::filesystem::path(output_file_path).extension().string();
    std::transform(input_extension.begin(), input_extension.end(), input_extension.begin(), ::tolower);
    std::transform(output_extension.begin(), output_extension.end(), output_extension.begin(), ::tolower);

    // 创建临时CSV文件
    std::string temp_csv_path = output_file_path + ".temp.csv";

    // 使用UTF-8编码打开输入文件
    std::ifstream input_file(input_file_path, std::ios::binary);
    if (!input_file.is_open()) {
        std::cerr << "无法打开输入Excel文件: " << input_file_path << std::endl;
        return false;
    }

    // 读取文件内容到字符串
    std::string content((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
    input_file.close();

    // 检查BOM标记并保留
    std::string bom = "";
    if (content.size() >= 3 &&
        (unsigned char)content[0] == 0xEF &&
        (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF) {
        bom = content.substr(0, 3); // 保留BOM
        content = content.substr(3);
    }

    std::stringstream ss(content);
    std::string line;
    bool first_line = true;

    // 写入BOM到临时CSV文件
    std::ofstream temp_file(temp_csv_path, std::ios::binary);
    if (!temp_file.is_open()) {
        std::cerr << "无法创建临时CSV文件: " << temp_csv_path << std::endl;
        return false;
    }

    // 写入BOM
    if (!bom.empty()) {
        temp_file.write(bom.c_str(), bom.size());
    }

    while (std::getline(ss, line)) {
        if (first_line) {
            // 写入标题行
            temp_file << line << std::endl;
            first_line = false;
            continue;
        }

        if (line.empty()) {
            temp_file << std::endl;
            continue;
        }

        ExcelRowData row_data = parse_csv_line(line);

        // 如果有对应的file_id，更新标签
        if (!row_data.content_id.empty() && tag_map.find(row_data.content_id) != tag_map.end()) {
            row_data.tags = tag_map[row_data.content_id];
        }

        // 重新组合行，确保标签列正确格式化
        std::string tags = row_data.tags;
        // 如果标签以'['开头，说明是列表格式，需要处理
        if (tags.length() > 0 && tags[0] == '[') {
            // 移除外层引号和括号
            if (tags.length() > 2 && tags[tags.length()-1] == ']') {
                tags = tags.substr(1, tags.length() - 2);
            }
            // 替换单引号为双引号
            size_t pos = 0;
            while ((pos = tags.find("'", pos)) != std::string::npos) {
                tags.replace(pos, 1, """);
                pos += 1;
            }
        }

        // 重新组合行
        temp_file << row_data.sequence << ","
                    << row_data.content_type << ","
                    << row_data.location << ","
                    << row_data.content_id << ","
                    << row_data.url << ","
                    << tags << std::endl;
    }

    temp_file.close();

    // 如果输出文件是xlsx格式，将CSV转换为XLSX
    if (output_extension == ".xlsx") {
        std::string command = "python3 -c '"
            "import pandas as pd; "
            "import re; "
            "df = pd.read_csv('" + temp_csv_path + "'); "
            "# 处理标签列，确保没有非法字符; "
            "if '标签' in df.columns: "
            "  df['标签'] = df['标签'].apply(lambda x: re.sub(r'[\x00-\x1F\x7F]', '', str(x)) if pd.notna(x) else x); "
            "df.to_excel('" + output_file_path + "', index=False, engine='openpyxl')'";

        std::cout << "执行XLSX转换命令: " << command << std::endl;
        int result = system(command.c_str());

        if (result != 0) {
            std::cerr << "CSV转XLSX失败，尝试使用libreoffice..." << std::endl;

            // 备用方案：使用libreoffice
            command = "libreoffice --headless --convert-to xlsx --outdir " + 
                      std::filesystem::path(output_file_path).parent_path().string() + " " + temp_csv_path;
            std::cout << "执行备用转换命令: " << command << std::endl;
            result = system(command.c_str());

            if (result != 0) {
                std::cerr << "无法将CSV转换为XLSX文件，请确保已安装python3 pandas或libreoffice" << std::endl;
                std::filesystem::remove(temp_csv_path);
                return false;
            }

            // libreoffice输出的文件名可能不同，需要查找
            std::string base_name = std::filesystem::path(temp_csv_path).stem().string();
            std::string libreoffice_output = std::filesystem::path(output_file_path).parent_path().string() + "/" + base_name + ".xlsx";

            // 如果libreoffice输出文件存在，重命名为目标文件
            if (std::filesystem::exists(libreoffice_output)) {
                std::filesystem::rename(libreoffice_output, output_file_path);
            }
        }
    } else {
        // 如果输出文件是CSV格式，直接复制
        std::filesystem::copy_file(temp_csv_path, output_file_path);
    }

    // 删除临时CSV文件
    std::filesystem::remove(temp_csv_path);

    std::cout << "成功更新Excel文件，输出到: " << output_file_path << std::endl;

    return true;
}
EOF

# 替换原文件
mv ExcelProcessor_fixed.cpp ExcelProcessor.cpp

echo "ExcelProcessor.cpp已修复"
