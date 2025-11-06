#include "DoubaoMediaAnalyzer.hpp"
#include "config.hpp"
#include <filesystem>
#include <iostream>

// 初始化数据库连接
bool DoubaoMediaAnalyzer::initialize_database()
{
    if (!db_manager_)
    {
        std::cerr << "数据库管理器未初始化" << std::endl;
        return false;
    }

    return db_manager_->initialize();
}

// 保存单个分析结果到数据库
bool DoubaoMediaAnalyzer::save_result_to_database(const AnalysisResult &result)
{
    if (!db_manager_)
    {
        std::cerr << "数据库管理器未初始化" << std::endl;
        return false;
    }

    if (!result.success)
    {
        std::cerr << "分析结果不成功，不保存到数据库" << std::endl;
        return false;
    }

    //
    const std::string &file_path = result.raw_response.contains("path") ? result.raw_response["path"] : "unknown";
    //
    MediaAnalysisRecord record;
    record.file_path = file_path;
    record.file_name = std::filesystem::path(file_path).filename().string();
    record.file_type = result.raw_response.contains("type") ? result.raw_response["type"] : "unknown";
    record.analysis_result = result.content;
    record.response_time = result.response_time;

    // 提取标签
    auto tags = extract_tags(result.content);
    if (!tags.empty())
    {
        std::string tags_str;
        for (size_t i = 0; i < tags.size(); ++i)
        {
            if (i > 0)
                tags_str += ", ";
            tags_str += tags[i];
        }
        record.tags = tags_str;
    }

    return db_manager_->save_analysis_result(record);
}

// 保存批量分析结果到数据库
bool DoubaoMediaAnalyzer::save_batch_results_to_database(const std::vector<AnalysisResult> &results)
{
    if (!db_manager_)
    {
        std::cerr << "数据库管理器未初始化" << std::endl;
        return false;
    }

    std::vector<MediaAnalysisRecord> records;

    for (size_t i = 0; i < results.size(); ++i)
    {
        if (!results[i].success)
        {
            continue; // 跳过失败的分析结果
        }

        //
        const std::string &file_path = results[i].raw_response.contains("path") ? results[i].raw_response["path"] : "unknown";
        //

        MediaAnalysisRecord record;
        record.file_path = file_path;
        record.file_name = std::filesystem::path(file_path).filename().string();
        record.file_type = results[i].raw_response.contains("type") ? results[i].raw_response["type"] : "unknown";
        record.analysis_result = results[i].content;
        record.response_time = results[i].response_time;

        // 提取标签
        auto tags = extract_tags(results[i].content);
        if (!tags.empty())
        {
            std::string tags_str;
            for (size_t j = 0; j < tags.size(); ++j)
            {
                if (j > 0)
                    tags_str += ", ";
                tags_str += tags[j];
            }
            record.tags = tags_str;
        }

        records.push_back(record);
    }

    return db_manager_->save_batch_results(records);
}

// 查询数据库结果
std::vector<MediaAnalysisRecord> DoubaoMediaAnalyzer::query_database_results(const std::string &condition)
{
    if (!db_manager_)
    {
        std::cerr << "数据库管理器未初始化" << std::endl;
        return {};
    }

    return db_manager_->query_results(condition);
}

// 按标签查询
std::vector<MediaAnalysisRecord> DoubaoMediaAnalyzer::query_by_tag(const std::string &tag)
{
    if (!db_manager_)
    {
        std::cerr << "数据库管理器未初始化" << std::endl;
        return {};
    }

    return db_manager_->query_by_tag(tag);
}

// 获取数据库统计信息
nlohmann::json DoubaoMediaAnalyzer::get_database_statistics()
{
    if (!db_manager_)
    {
        std::cerr << "数据库管理器未初始化" << std::endl;
        return {};
    }

    return db_manager_->get_statistics();
}
