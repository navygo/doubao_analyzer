#pragma once

#include <string>
#include <nlohmann/json.hpp>

// 数据库配置结构
struct DatabaseConfig
{
    std::string host;
    std::string user;
    std::string password;
    std::string database;
    unsigned int port = 3306;
    std::string charset = "utf8mb4";
    int connection_timeout = 60;
    int read_timeout = 60;
    int write_timeout = 60;
};

// 备份配置结构
struct BackupConfig
{
    bool auto_backup;
    int backup_interval_hours;
    int backup_retention_days;
    std::string backup_path;

    BackupConfig() : auto_backup(true), backup_interval_hours(24),
                     backup_retention_days(30), backup_path("./backups/") {}
};

// 清理配置结构
struct CleanupConfig
{
    bool auto_cleanup;
    int retention_days;

    CleanupConfig() : auto_cleanup(true), retention_days(90) {}
};

class ConfigManager
{
private:
    std::string config_file_path_;
    DatabaseConfig db_config_;
    BackupConfig backup_config_;
    CleanupConfig cleanup_config_;

    // 解析JSON配置
    void parse_config(const nlohmann::json &config);

    // 生成默认配置
    nlohmann::json get_default_config();

public:
    // 构造函数
    explicit ConfigManager(const std::string &config_file = "./config/db_config.json");

    // 析构函数
    virtual ~ConfigManager() = default;

    // 加载配置
    bool load_config();

    // 保存配置
    bool save_config();

    // 获取数据库配置
    const DatabaseConfig &get_database_config() const;

    // 获取备份配置
    const BackupConfig &get_backup_config() const;

    // 获取清理配置
    const CleanupConfig &get_cleanup_config() const;

    // 设置数据库配置
    void set_database_config(const DatabaseConfig &config);

    // 设置备份配置
    void set_backup_config(const BackupConfig &config);

    // 设置清理配置
    void set_cleanup_config(const CleanupConfig &config);
};
