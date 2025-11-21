#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "DatabaseManager.hpp"
#include "ConfigManager.hpp"
#include "VideoKeyframeAnalyzer.hpp"
#include "utils.hpp"
#include "config.hpp"

struct AnalysisResult
{
    bool success;
    std::string content;
    double response_time;
    nlohmann::json usage;
    nlohmann::json raw_response;
    std::string error;

    AnalysisResult() : success(false), response_time(0.0) {}
};

class DoubaoMediaAnalyzer
{
private:
    std::string api_key_;
    std::string base_url_;
    std::string model_name_;
    std::unique_ptr<DatabaseManager> db_manager_;
    std::unique_ptr<VideoKeyframeAnalyzer> video_analyzer_;
    bool use_ollama_; // 标识是否使用Ollama API

    // 判断是否使用Ollama API
    bool is_ollama_api(const std::string &url) const;

public:
    // 使用默认配置构造函数
    explicit DoubaoMediaAnalyzer(const std::string &api_key);

    // 使用自定义API配置构造函数
    DoubaoMediaAnalyzer(const std::string &api_key, const std::string &base_url, const std::string &model_name);

    // 使用ApiConfig结构体构造函数
    explicit DoubaoMediaAnalyzer(const config::ApiConfig &api_config);

    // 连接测试
    bool test_connection();

    // 单张图片分析
    AnalysisResult analyze_single_image(const std::string &image_path,
                                        const std::string &prompt,
                                        int max_tokens = 1500,
                                        const std::string &model_name = "");

    // 单个视频分析
    AnalysisResult analyze_single_video(const std::string &video_path,
                                        const std::string &prompt,
                                        int max_tokens = 2000,
                                        int num_frames = 5,
                                        const std::string &model_name = "");

    // 高效视频分析（使用关键帧，无需完整下载）
    AnalysisResult analyze_video_efficiently(const std::string &video_url,
                                             const std::string &prompt,
                                             int max_tokens = 2000,
                                             const std::string &method = "keyframes",
                                             int num_frames = 5,
                                             const std::string &model_name = "");

    // 批量分析
    std::vector<AnalysisResult> batch_analyze(const std::string &media_folder,
                                              const std::string &prompt,
                                              int max_files = 5,
                                              const std::string &file_type = "all");

    // 标签提取
    std::vector<std::string> extract_tags(const std::string &content);

    // 数据库操作
    bool initialize_database();
    bool save_result_to_database(const AnalysisResult &result);
    bool save_batch_results_to_database(const std::vector<AnalysisResult> &results);
    std::vector<MediaAnalysisRecord> query_database_results(const std::string &condition = "");
    std::vector<MediaAnalysisRecord> query_by_tag(const std::string &tag);
    std::vector<MediaAnalysisRecord> query_by_url(const std::string &media_url);
    std::vector<MediaAnalysisRecord> query_by_type(const std::string &file_type);
    std::vector<MediaAnalysisRecord> query_by_date_range(const std::string &start_date, const std::string &end_date);
    std::vector<MediaAnalysisRecord> get_recent_results(int limit = 10);
    nlohmann::json get_database_statistics();

    virtual ~DoubaoMediaAnalyzer(); // 添加这行

private:
    // 内部方法
    std::vector<std::string> extract_video_frames(const std::string &video_path, int num_frames);
    AnalysisResult send_analysis_request(const nlohmann::json &payload, int timeout);
    AnalysisResult process_response(const std::string &response_text, double response_time);

    // HTTP请求
    std::string make_http_request(const std::string &url,
                                  const std::string &method,
                                  const std::string &data,
                                  const std::vector<std::string> &headers,
                                  int timeout,
                                  bool enable_http2);
};
