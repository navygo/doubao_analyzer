#include "ConfigManager.hpp"
#include "utils.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

ConfigManager::ConfigManager(const std::string &config_file)
    : config_file_path_(config_file)
{
    // 使用默认值初始化
    db_config_ = DatabaseConfig();
    backup_config_ = BackupConfig();
    cleanup_config_ = CleanupConfig();
    auth_config_ = AuthConfig();
}

bool ConfigManager::load_config()
{
    try
    {
        std::ifstream file(config_file_path_);
        if (!file.is_open())
        {
            std::cout << "配置文件不存在，将创建默认配置文件: " << config_file_path_ << std::endl;
            return save_config(); // 创建默认配置文件
        }

        nlohmann::json config;
        file >> config;
        file.close();

        parse_config(config);
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "加载配置文件失败: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::save_config()
{
    try
    {
        nlohmann::json config = get_default_config();

        // 更新配置值
        config["database"]["host"] = db_config_.host;
        config["database"]["port"] = db_config_.port;
        config["database"]["user"] = db_config_.user;
        config["database"]["password"] = db_config_.password;
        config["database"]["name"] = db_config_.database;
        config["database"]["charset"] = db_config_.charset;
        config["database"]["connection_timeout"] = db_config_.connection_timeout;
        config["database"]["read_timeout"] = db_config_.read_timeout;
        config["database"]["write_timeout"] = db_config_.write_timeout;

        // 连接池配置
        config["database"]["pool"]["initial_size"] = db_config_.pool_config.initial_size;
        config["database"]["pool"]["max_size"] = db_config_.pool_config.max_size;
        config["database"]["pool"]["max_idle_time"] = db_config_.pool_config.max_idle_time;
        config["database"]["pool"]["wait_timeout"] = db_config_.pool_config.wait_timeout;
        config["database"]["pool"]["auto_reconnect"] = db_config_.pool_config.auto_reconnect;

        config["backup"]["auto_backup"] = backup_config_.auto_backup;
        config["backup"]["backup_interval_hours"] = backup_config_.backup_interval_hours;
        config["backup"]["backup_retention_days"] = backup_config_.backup_retention_days;
        config["backup"]["backup_path"] = backup_config_.backup_path;

        config["cleanup"]["auto_cleanup"] = cleanup_config_.auto_cleanup;
        config["cleanup"]["retention_days"] = cleanup_config_.retention_days;

        config["auth"]["admin_user"] = auth_config_.admin_user;
        config["auth"]["admin_pass"] = auth_config_.admin_pass;

        // 确保目录存在
        std::filesystem::path config_path(config_file_path_);
        std::filesystem::path config_dir = config_path.parent_path();
        if (!config_dir.empty() && !std::filesystem::exists(config_dir))
        {
            std::filesystem::create_directories(config_dir);
        }

        // 写入文件
        std::ofstream file(config_file_path_);
        file << config.dump(4) << std::endl;
        file.close();

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "保存配置文件失败: " << e.what() << std::endl;
        return false;
    }
}

const DatabaseConfig &ConfigManager::get_database_config() const
{
    return db_config_;
}

const BackupConfig &ConfigManager::get_backup_config() const
{
    return backup_config_;
}

const CleanupConfig &ConfigManager::get_cleanup_config() const
{
    return cleanup_config_;
}

const AuthConfig &ConfigManager::get_auth_config() const
{
    return auth_config_;
}

void ConfigManager::set_database_config(const DatabaseConfig &config)
{
    db_config_ = config;
}

void ConfigManager::set_backup_config(const BackupConfig &config)
{
    backup_config_ = config;
}

void ConfigManager::set_cleanup_config(const CleanupConfig &config)
{
    cleanup_config_ = config;
}

void ConfigManager::set_auth_config(const AuthConfig &config)
{
    auth_config_ = config;
}

void ConfigManager::parse_config(const nlohmann::json &config)
{
    // 解析数据库配置
    if (config.contains("database"))
    {
        const auto &db = config["database"];
        if (db.contains("host"))
            db_config_.host = db["host"];
        if (db.contains("port"))
            db_config_.port = db["port"];
        if (db.contains("user"))
            db_config_.user = db["user"];
        if (db.contains("password"))
            db_config_.password = db["password"];
        if (db.contains("name"))
            db_config_.database = db["name"];
        if (db.contains("charset"))
            db_config_.charset = db["charset"];
        if (db.contains("connection_timeout"))
            db_config_.connection_timeout = db["connection_timeout"];
        if (db.contains("read_timeout"))
            db_config_.read_timeout = db["read_timeout"];
        if (db.contains("write_timeout"))
            db_config_.write_timeout = db["write_timeout"];

        // 解析连接池配置
        if (db.contains("pool"))
        {
            const auto &pool = db["pool"];
            if (pool.contains("initial_size"))
                db_config_.pool_config.initial_size = pool["initial_size"];
            if (pool.contains("max_size"))
                db_config_.pool_config.max_size = pool["max_size"];
            if (pool.contains("max_idle_time"))
                db_config_.pool_config.max_idle_time = pool["max_idle_time"];
            if (pool.contains("wait_timeout"))
                db_config_.pool_config.wait_timeout = pool["wait_timeout"];
            if (pool.contains("auto_reconnect"))
                db_config_.pool_config.auto_reconnect = pool["auto_reconnect"];
        }
    }

    // 解析备份配置
    if (config.contains("backup"))
    {
        const auto &backup = config["backup"];
        if (backup.contains("auto_backup"))
            backup_config_.auto_backup = backup["auto_backup"];
        if (backup.contains("backup_interval_hours"))
            backup_config_.backup_interval_hours = backup["backup_interval_hours"];
        if (backup.contains("backup_retention_days"))
            backup_config_.backup_retention_days = backup["backup_retention_days"];
        if (backup.contains("backup_path"))
            backup_config_.backup_path = backup["backup_path"];
    }

    // 解析清理配置
    if (config.contains("cleanup"))
    {
        const auto &cleanup = config["cleanup"];
        if (cleanup.contains("auto_cleanup"))
            cleanup_config_.auto_cleanup = cleanup["auto_cleanup"];
        if (cleanup.contains("retention_days"))
            cleanup_config_.retention_days = cleanup["retention_days"];
    }

    // 解析认证配置
    if (config.contains("auth"))
    {
        const auto &auth = config["auth"];
        if (auth.contains("admin_user"))
            auth_config_.admin_user = auth["admin_user"];
        if (auth.contains("admin_pass"))
            auth_config_.admin_pass = auth["admin_pass"];
    }
}

nlohmann::json ConfigManager::get_default_config()
{
    nlohmann::json config;

    // 数据库默认配置
    config["database"]["host"] = "localhost";
    config["database"]["port"] = 3306;
    config["database"]["user"] = "root";
    config["database"]["password"] = "password";
    config["database"]["name"] = "doubao_analyzer";
    config["database"]["charset"] = "utf8mb4";
    config["database"]["connection_timeout"] = 60;
    config["database"]["read_timeout"] = 60;
    config["database"]["write_timeout"] = 60;

    // 连接池默认配置
    config["database"]["pool"]["initial_size"] = 5;
    config["database"]["pool"]["max_size"] = 20;
    config["database"]["pool"]["max_idle_time"] = 300;
    config["database"]["pool"]["wait_timeout"] = 5000;
    config["database"]["pool"]["auto_reconnect"] = true;

    // 备份默认配置
    config["backup"]["auto_backup"] = true;
    config["backup"]["backup_interval_hours"] = 24;
    config["backup"]["backup_retention_days"] = 30;
    config["backup"]["backup_path"] = "./backups/";

    // 清理默认配置
    config["cleanup"]["auto_cleanup"] = true;
    config["cleanup"]["retention_days"] = 90;

    // 认证默认配置
    config["auth"]["admin_user"] = "admin";
    config["auth"]["admin_pass"] = "password";

    return config;
}
