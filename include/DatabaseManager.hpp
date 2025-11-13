#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include <mysql/mysql.h>
#include "ConfigManager.hpp"

// 媒体分析结果记录
struct MediaAnalysisRecord
{
    int id;
    std::string file_path;
    std::string file_name;
    std::string file_type; // "image" or "video"
    std::string analysis_result;
    std::string tags;
    double response_time;
    std::string created_at;

    MediaAnalysisRecord() : id(0), response_time(0.0) {}
};

class DatabaseManager
{
private:
    MYSQL *connection_;
    std::string host_;
    std::string user_;
    std::string password_;
    std::string database_;
    unsigned int port_;
    std::string charset_;
    int connection_timeout_;
    int read_timeout_;
    int write_timeout_;

    // 连接到数据库
    bool connect();

    // 执行SQL语句
    bool execute_query(const std::string &query);

    // 初始化数据库表
    bool initialize_tables();

    // 获取当前时间戳
    std::string get_current_timestamp();

public:
    // 构造函数
    DatabaseManager(const std::string &host, const std::string &user,
                    const std::string &password, const std::string &database,
                    unsigned int port = 3306, const std::string &charset = "utf8mb4",
                    int cocalnnection_timeout = 60, int read_timeout = 60, int write_timeout = 60);

    // 从配置构造
    DatabaseManager(const DatabaseConfig &config);

    // 析构函数
    virtual ~DatabaseManager();

    // 初始化数据库连接
    bool initialize();

    // 关闭数据库连接
    void close();

    // 保存分析结果
    bool save_analysis_result(const MediaAnalysisRecord &record);

    // 保存批量分析结果
    bool save_batch_results(const std::vector<MediaAnalysisRecord> &records);

    // 查询分析结果
    std::vector<MediaAnalysisRecord> query_results(const std::string &condition = "");

    // 根据标签查询
    std::vector<MediaAnalysisRecord> query_by_tag(const std::string &tag);

    // 根据文件类型查询
    std::vector<MediaAnalysisRecord> query_by_type(const std::string &file_type);

    // 根据时间范围查询
    std::vector<MediaAnalysisRecord> query_by_date_range(const std::string &start_date, const std::string &end_date);

    // 获取最近的分析结果
    std::vector<MediaAnalysisRecord> get_recent_results(int limit = 10);

    // 获取统计信息
    nlohmann::json get_statistics();

    // --- Refresh token management ---
    // 创建 refresh token 记录（token_hash 为 SHA256 hex）
    bool create_refresh_token_record(const std::string &token_hash, const std::string &user_id, long created_at, long expires_at);

    // 验证 refresh token，返回 true 并填充 out_user_id
    bool verify_refresh_token_record(const std::string &token_hash, std::string &out_user_id);

    // 撤销 refresh token（删除记录）
    bool revoke_refresh_token_record(const std::string &token_hash);

    // 备份数据库
    bool backup_database(const std::string &backup_path);

    // 清理旧记录
    bool cleanup_old_records(int days_to_keep);
};
