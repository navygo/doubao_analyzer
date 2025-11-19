#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "TaskManager.hpp"

// Excel行数据结构
struct ExcelRowData
{
    std::string sequence;      // 序号
    std::string content_type;  // 内容类型 (图片/视频)
    std::string location;       // 内容发布位置
    std::string content_id;    // 内容id
    std::string url;           // 内容地址
    std::string tags;          // 标签

    ExcelRowData() = default;
};

// Excel处理器类
class ExcelProcessor
{
private:
    // 解析CSV格式的一行
    ExcelRowData parse_csv_line(const std::string &line);

public:
    // 构造函数
    ExcelProcessor() = default;

    // 析构函数
    ~ExcelProcessor() = default;

    // 从Excel文件(CSV或XLSX格式)读取数据
    std::vector<ExcelRowData> read_excel_file(const std::string &file_path);

    // 将Excel数据转换为分析任务
    std::vector<AnalysisTask> create_analysis_tasks(
        const std::vector<ExcelRowData> &excel_data,
        const std::string &prompt,
        int max_tokens = 1500,
        bool save_to_db = true);

    // 更新Excel文件中的标签列
    bool update_excel_tags(
        const std::string &input_file_path,
        const std::string &output_file_path,
        const std::vector<std::pair<std::string, std::string>> &file_id_tags);

    // 从数据库读取媒体数据
    std::vector<ExcelRowData> read_media_from_db();

    // 批量分析数据库中的媒体
    std::vector<AnalysisTask> analyze_db_media(
        const std::string &prompt,
        int max_tokens = 1500,
        bool save_to_db = true);

private:
    // 从XLSX文件读取数据
    std::vector<ExcelRowData> read_xlsx_file(const std::string &file_path);

    // 从CSV文件读取数据
    std::vector<ExcelRowData> read_csv_file(const std::string &file_path);
};
