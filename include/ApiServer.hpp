#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "DoubaoMediaAnalyzer.hpp"
#include "TaskManager.hpp"
#include "ExcelProcessor.hpp"

// API请求结构
struct ApiRequest
{
    std::string media_type; // "image" or "video"
    std::string media_url;  // 图片或视频的URL地址
    std::string prompt;     // 可选的自定义提示词
    int max_tokens;         // 可选的最大令牌数
    int video_frames;       // 可选的视频帧数（仅视频分析）
    bool save_to_db;        // 是否保存结果到数据库

    // 大模型配置参数 如果Ollama本地部署模型 可以选择模型
    std::string model_name; // 大模型名称
};

// API查询请求结构
struct ApiQueryRequest
{
    std::string query_type; // "all", "tag", "type", "date_range", "recent", "url"
    std::string tag;        // 要查询的标签（当query_type为"tag"时使用）
    std::string file_type;  // 要查询的文件类型（当query_type为"type"时使用）
    std::string start_date; // 开始日期（当query_type为"date_range"时使用）
    std::string end_date;   // 结束日期（当query_type为"date_range"时使用）
    int limit;              // 返回结果数量限制（当query_type为"recent"时使用）
    std::string condition;  // 自定义查询条件
    std::string media_url;  // 要查询的媒体URL（当query_type为"url"时使用）
};

// Excel处理请求结构
struct ApiExcelRequest
{
    std::string excel_path;  // Excel文件路径
    std::string output_path; // 输出Excel文件路径
    std::string prompt;      // 分析提示词
    int max_tokens;          // 最大令牌数
    bool save_to_db;         // 是否保存到数据库
};

// API响应结构
struct ApiResponse
{
    bool success;
    std::string message;
    nlohmann::json data;
    double response_time;
    std::string error;

    ApiResponse() : success(false), response_time(0.0) {}
};

class ApiServer
{
private:
    std::string api_key_;
    std::unique_ptr<DoubaoMediaAnalyzer> analyzer_;
    int port_;
    std::string host_;

    // 解析API请求
    ApiResponse parse_request(const std::string &request_json, const std::string &path);

    // 处理图片分析请求
    ApiResponse handle_image_analysis(const ApiRequest &request);

    // 处理视频分析请求
    ApiResponse handle_video_analysis(const ApiRequest &request);

    // 处理查询请求
    ApiResponse handle_query_request(const ApiQueryRequest &request);

    // 处理批量分析请求
    ApiResponse handle_batch_analysis(const std::vector<ApiRequest> &requests);

    // 处理Excel文件分析请求
    ApiResponse handle_excel_analysis(const ApiExcelRequest &request);

    // 处理数据库媒体分析请求
    ApiResponse handle_db_media_analysis(const std::string &prompt, int max_tokens = 1500, bool save_to_db = true);

    // 将结果保存到数据库
    bool save_to_database(const AnalysisResult &result, const std::string &media_url, const std::string &media_type);
    // 将批量结果保存到数据库
    bool save_batch_to_database(const std::vector<AnalysisResult> &results);

public:
    ApiServer(const std::string &api_key, int port = 8080, const std::string &host = "0.0.0.0");

    // 初始化API服务器
    bool initialize();

    // 启动HTTP服务器
    void start();

    // 停止服务器
    void stop();

    // 处理API请求
    // auth_header: 来自HTTP头部的 Authorization 字段值（例如 "Bearer <token>"）
    ApiResponse process_request(const std::string &request_json, const std::string &path = "/", const std::string &auth_header = "");

    // 获取服务器状态
    nlohmann::json get_status();

    ~ApiServer();
};
