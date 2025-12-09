#pragma once

#include <string>
#include <nlohmann/json.hpp>

// 连接池配置结构
struct PoolConfig
{
    size_t initial_size = 5;
    size_t max_size = 20;
    size_t max_idle_time = 300;
    size_t wait_timeout = 5000;
    bool auto_reconnect = true;
};

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
    PoolConfig pool_config;
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

// 认证配置
struct AuthConfig
{
    std::string admin_user;
    std::string admin_pass;

    AuthConfig() : admin_user("admin"), admin_pass("password") {}
};

class ConfigManager
{
private:
    std::string config_file_path_;
    DatabaseConfig db_config_;
    BackupConfig backup_config_;
    CleanupConfig cleanup_config_;
    AuthConfig auth_config_;

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

    // 获取认证配置
    const AuthConfig &get_auth_config() const;

    // 设置数据库配置
    void set_database_config(const DatabaseConfig &config);

    // 设置备份配置
    void set_backup_config(const BackupConfig &config);

    // 设置清理配置
    void set_cleanup_config(const CleanupConfig &config);
    void set_auth_config(const AuthConfig &config);
};
