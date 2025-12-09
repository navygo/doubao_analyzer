#include "DatabaseManager.hpp"
#include "utils.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <thread>
#include <mysql.h>

DatabaseManager::DatabaseManager(const std::string &host, const std::string &user,
                                 const std::string &password, const std::string &database,
                                 unsigned int port, const std::string &charset,
                                 int connection_timeout, int read_timeout, int write_timeout,
                                 size_t pool_initial_size, size_t pool_max_size,
                                 size_t pool_max_idle_time, size_t pool_wait_timeout,
                                 bool pool_auto_reconnect)
    : connection_pool_(nullptr), host_(host), user_(user), password_(password),
      database_(database), port_(port), charset_(charset),
      connection_timeout_(connection_timeout), read_timeout_(read_timeout), write_timeout_(write_timeout),
      pool_initial_size_(pool_initial_size), pool_max_size_(pool_max_size),
      pool_max_idle_time_(pool_max_idle_time), pool_wait_timeout_(pool_wait_timeout),
      pool_auto_reconnect_(pool_auto_reconnect)
{
}

DatabaseManager::DatabaseManager(const DatabaseConfig &config)
    : connection_pool_(nullptr), host_(config.host), user_(config.user), password_(config.password),
      database_(config.database), port_(config.port), charset_(config.charset),
      connection_timeout_(config.connection_timeout), read_timeout_(config.read_timeout),
      write_timeout_(config.write_timeout),
      pool_initial_size_(5), pool_max_size_(20), pool_max_idle_time_(300),
      pool_wait_timeout_(5000), pool_auto_reconnect_(true)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::initialize()
{
    // 创建数据库配置
    DatabaseConfig db_config;
    db_config.host = host_;
    db_config.user = user_;
    db_config.password = password_;
    db_config.database = database_;
    db_config.port = port_;
    db_config.charset = charset_;
    db_config.connection_timeout = connection_timeout_;
    db_config.read_timeout = read_timeout_;
    db_config.write_timeout = write_timeout_;
    db_config.pool_config.initial_size = pool_initial_size_;
    db_config.pool_config.max_size = pool_max_size_;
    db_config.pool_config.max_idle_time = pool_max_idle_time_;
    db_config.pool_config.wait_timeout = pool_wait_timeout_;
    db_config.pool_config.auto_reconnect = pool_auto_reconnect_;

    // 创建连接池
    connection_pool_ = std::make_shared<DatabaseConnectionPool>(db_config, db_config.pool_config);

    // 初始化连接池
    if (!connection_pool_->initialize())
    {
        std::cerr << "Failed to initialize database connection pool" << std::endl;
        return false;
    }

    if (!initialize_tables())
    {
        std::cerr << "Failed to initialize database tables" << std::endl;
        return false;
    }

    return true;
}

// 此方法不再需要，连接池内部管理连接

void DatabaseManager::close()
{
    if (connection_pool_)
    {
        connection_pool_->shutdown();
        connection_pool_.reset();
    }
}

bool DatabaseManager::execute_query(const std::string &query)
{
    if (!connection_pool_ || !connection_pool_->is_valid())
    {
        std::cerr << "Database connection pool is not initialized" << std::endl;
        return false;
    }

    // 从连接池获取连接
    ConnectionWrapper conn_wrapper = connection_pool_->get_connection();
    MYSQL *connection = conn_wrapper.get();

    if (connection == nullptr)
    {
        std::cerr << "Failed to get database connection from pool" << std::endl;
        return false;
    }

    if (mysql_query(connection, query.c_str()) != 0)
    {
        std::cerr << "MySQL query error: " << mysql_error(connection) << std::endl;
        std::cerr << "Query: " << query << std::endl;
        return false;
    }

    return true;
}

bool DatabaseManager::initialize_tables()
{
    // 创建媒体分析结果表
    std::string create_table_query = R"(
        CREATE TABLE IF NOT EXISTS media_analysis (
            id INT AUTO_INCREMENT PRIMARY KEY,
            file_path VARCHAR(1024) NOT NULL,
            file_name VARCHAR(255) NOT NULL,
            file_type ENUM('image', 'video') NOT NULL,
            analysis_result TEXT,
            tags VARCHAR(1024),
            response_time DOUBLE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            file_id VARCHAR(255),
            INDEX idx_file_type (file_type),
            INDEX idx_created_at (created_at),
            INDEX idx_tags (tags(255)),
            INDEX idx_file_id (file_id)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
    )";

    if (!execute_query(create_table_query))
        return false;

    // 创建 refresh_tokens 表
    std::string create_refresh_table = R"(
        CREATE TABLE IF NOT EXISTS refresh_tokens (
            token_hash CHAR(64) PRIMARY KEY,
            user_id VARCHAR(128) NOT NULL,
            created_at BIGINT NOT NULL,
            expires_at BIGINT NOT NULL,
            last_used BIGINT DEFAULT NULL,
            INDEX idx_expires_at (expires_at)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
    )";

    if (!execute_query(create_refresh_table))
        return false;

    return true;
}

std::string DatabaseManager::get_current_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

bool DatabaseManager::create_refresh_token_record(const std::string &token_hash, const std::string &user_id, long created_at, long expires_at)
{
    std::stringstream q;
    q << "INSERT INTO refresh_tokens (token_hash, user_id, created_at, expires_at) VALUES ('";
    q << utils::replace_all(token_hash, "'", "''") << "', '";
    q << utils::replace_all(user_id, "'", "''") << "', ";
    q << created_at << ", " << expires_at << ")";
    return execute_query(q.str());
}

bool DatabaseManager::verify_refresh_token_record(const std::string &token_hash, std::string &out_user_id)
{
    if (!connection_pool_ || !connection_pool_->is_valid())
        return false;

    // 从连接池获取连接
    ConnectionWrapper conn_wrapper = connection_pool_->get_connection();
    MYSQL *connection = conn_wrapper.get();

    if (connection == nullptr)
        return false;

    std::string q = "SELECT user_id, expires_at FROM refresh_tokens WHERE token_hash = '" + utils::replace_all(token_hash, "'", "''") + "' LIMIT 1";
    if (mysql_query(connection, q.c_str()) != 0)
        return false;

    MYSQL_RES *result = mysql_store_result(connection);
    if (result == nullptr)
        return false;
    MYSQL_ROW row = mysql_fetch_row(result);
    bool ok = false;
    if (row != nullptr)
    {
        std::string user = row[0] ? row[0] : "";
        long expires_at = row[1] ? atol(row[1]) : 0;
        long now = (long)std::time(nullptr);
        if (now <= expires_at)
        {
            out_user_id = user;
            ok = true;
        }
    }
    mysql_free_result(result);
    return ok;
}

bool DatabaseManager::revoke_refresh_token_record(const std::string &token_hash)
{
    std::string q = "DELETE FROM refresh_tokens WHERE token_hash = '" + utils::replace_all(token_hash, "'", "''") + "'";
    return execute_query(q);
}

bool DatabaseManager::save_analysis_result(const MediaAnalysisRecord &record)
{
    std::stringstream query;
    query << "INSERT INTO media_analysis (file_path, file_name, file_type, analysis_result, tags, response_time, file_id) VALUES (";
    query << "'" << utils::replace_all(record.file_path, "'", "''") << "', ";
    query << "'" << utils::replace_all(record.file_name, "'", "''") << "', ";
    query << "'" << record.file_type << "', ";
    query << "'" << utils::replace_all(record.analysis_result, "'", "''") << "', ";
    query << "'" << utils::replace_all(record.tags, "'", "''") << "', ";
    query << std::fixed << std::setprecision(6) << record.response_time << ", ";
    query << "'" << utils::replace_all(record.file_id, "'", "''") << "'";
    query << ")";

    return execute_query(query.str());
}

bool DatabaseManager::save_batch_results(const std::vector<MediaAnalysisRecord> &records)
{
    if (records.empty())
    {
        return true;
    }

    // 最大重试次数
    const int max_retries = 3;
    int retry_count = 0;

    // 分批处理大小，减小每批次的记录数量以降低内存压力
    const int batch_size = 5;

    while (retry_count < max_retries)
    {
        try
        {
            // 检查连接池是否有效
            if (!connection_pool_ || !connection_pool_->is_valid())
            {
                std::cerr << "数据库连接池无效，尝试重新初始化..." << std::endl;
                // 先关闭现有连接池
                close();
                // 重新初始化连接池
                if (!initialize())
                {
                    std::cerr << "重新初始化数据库连接池失败" << std::endl;
                    retry_count++;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }
            }

            // 开始事务
            if (!execute_query("START TRANSACTION"))
            {
                std::cerr << "开始事务失败" << std::endl;
                retry_count++;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            bool all_success = true;

            // 分批处理记录
            for (size_t i = 0; i < records.size(); i += batch_size)
            {
                size_t end = std::min(i + batch_size, records.size());

                for (size_t j = i; j < end; ++j)
                {
                    if (!save_analysis_result(records[j]))
                    {
                        all_success = false;
                        std::cerr << "保存记录失败: " << records[j].file_name << std::endl;
                        break;
                    }
                }

                // 如果当前批次失败，跳出循环
                if (!all_success)
                {
                    break;
                }

                // 每处理一个小批次后提交一次，减少内存占用
                if (!execute_query("COMMIT"))
                {
                    std::cerr << "提交批次失败" << std::endl;
                    all_success = false;
                    break;
                }

                // 如果还有更多记录要处理，开始新事务
                if (end < records.size())
                {
                    if (!execute_query("START TRANSACTION"))
                    {
                        std::cerr << "开始新事务失败" << std::endl;
                        all_success = false;
                        break;
                    }
                }
            }

            if (all_success)
            {
                return true;
            }
            else
            {
                std::cerr << "保存记录失败" << std::endl;
            }

            // 回滚事务
            execute_query("ROLLBACK");
        }
        catch (const std::exception &e)
        {
            std::cerr << "批量保存异常: " << e.what() << std::endl;
            // 确保事务被回滚
            try
            {
                execute_query("ROLLBACK");
            }
            catch (...)
            {
                // 忽略回滚错误
            }

            // 重置连接，防止连接状态异常
            close();
        }

        retry_count++;
        if (retry_count < max_retries)
        {
            std::cerr << "批量保存失败，等待后重试 (" << retry_count << "/" << max_retries << ")" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    std::cerr << "批量保存失败，已达到最大重试次数" << std::endl;
    return false;
}

std::vector<MediaAnalysisRecord> DatabaseManager::query_results(const std::string &condition)
{
    std::vector<MediaAnalysisRecord> results;

    std::string query = "SELECT id, file_path, file_name, file_type, analysis_result, tags, response_time, created_at, file_id FROM media_analysis";
    if (!condition.empty())
    {
        query += " WHERE " + condition;
    }
    query += " ORDER BY created_at DESC";

    // 从连接池获取连接
    if (!connection_pool_ || !connection_pool_->is_valid())
    {
        std::cerr << "Database connection pool is not initialized" << std::endl;
        return results;
    }

    ConnectionWrapper conn_wrapper = connection_pool_->get_connection();
    MYSQL *connection = conn_wrapper.get();

    if (connection == nullptr)
    {
        std::cerr << "Failed to get database connection from pool" << std::endl;
        return results;
    }

    if (mysql_query(connection, query.c_str()) != 0)
    {
        std::cerr << "MySQL query error: " << mysql_error(connection) << std::endl;
        std::cerr << "Query: " << query << std::endl;
        return results;
    }

    MYSQL_RES *result = mysql_store_result(connection);
    if (result == nullptr)
    {
        return results;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr)
    {
        MediaAnalysisRecord record;
        record.id = atoi(row[0]);
        record.file_path = row[1] ? row[1] : "";
        record.file_name = row[2] ? row[2] : "";
        record.file_type = row[3] ? row[3] : "";
        record.analysis_result = row[4] ? row[4] : "";
        record.tags = row[5] ? row[5] : "";
        record.response_time = row[6] ? atof(row[6]) : 0.0;
        record.created_at = row[7] ? row[7] : "";
        record.file_id = row[8] ? row[8] : "";

        results.push_back(record);
    }

    mysql_free_result(result);
    return results;
}

std::vector<MediaAnalysisRecord> DatabaseManager::query_by_tag(const std::string &tag)
{
    return query_results("tags LIKE '%" + tag + "%'");
}

std::vector<MediaAnalysisRecord> DatabaseManager::query_by_type(const std::string &file_type)
{
    return query_results("file_type = '" + file_type + "'");
}

std::vector<MediaAnalysisRecord> DatabaseManager::get_recent_results(int limit)
{
    std::vector<MediaAnalysisRecord> results;

    std::stringstream query;
    query << "SELECT id, file_path, file_name, file_type, analysis_result, tags, response_time, created_at, file_id FROM media_analysis";
    query << " ORDER BY created_at DESC LIMIT " << limit;

    // 从连接池获取连接
    if (!connection_pool_ || !connection_pool_->is_valid())
    {
        std::cerr << "Database connection pool is not initialized" << std::endl;
        return results;
    }

    ConnectionWrapper conn_wrapper = connection_pool_->get_connection();
    MYSQL *connection = conn_wrapper.get();

    if (connection == nullptr)
    {
        std::cerr << "Failed to get database connection from pool" << std::endl;
        return results;
    }

    if (mysql_query(connection, query.str().c_str()) != 0)
    {
        std::cerr << "MySQL query error: " << mysql_error(connection) << std::endl;
        std::cerr << "Query: " << query.str() << std::endl;
        return results;
    }

    MYSQL_RES *result = mysql_store_result(connection);
    if (result == nullptr)
    {
        return results;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr)
    {
        MediaAnalysisRecord record;
        record.id = atoi(row[0]);
        record.file_path = row[1] ? row[1] : "";
        record.file_name = row[2] ? row[2] : "";
        record.file_type = row[3] ? row[3] : "";
        record.analysis_result = row[4] ? row[4] : "";
        record.tags = row[5] ? row[5] : "";
        record.response_time = row[6] ? atof(row[6]) : 0.0;
        record.created_at = row[7] ? row[7] : "";
        record.file_id = row[8] ? row[8] : "";

        results.push_back(record);
    }

    mysql_free_result(result);
    return results;
}

nlohmann::json DatabaseManager::get_statistics()
{
    nlohmann::json stats;

    // 从连接池获取连接
    if (!connection_pool_ || !connection_pool_->is_valid())
    {
        std::cerr << "Database connection pool is not initialized" << std::endl;
        return stats;
    }

    ConnectionWrapper conn_wrapper = connection_pool_->get_connection();
    MYSQL *connection = conn_wrapper.get();

    if (connection == nullptr)
    {
        std::cerr << "Failed to get database connection from pool" << std::endl;
        return stats;
    }

    // 总分析数量
    if (mysql_query(connection, "SELECT COUNT(*) FROM media_analysis") == 0)
    {
        MYSQL_RES *result = mysql_store_result(connection);
        if (result != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row != nullptr && row[0] != nullptr)
            {
                stats["total_analyses"] = atoi(row[0]);
            }
            mysql_free_result(result);
        }
    }

    // 图片分析数量
    if (mysql_query(connection, "SELECT COUNT(*) FROM media_analysis WHERE file_type = 'image'") == 0)
    {
        MYSQL_RES *result = mysql_store_result(connection);
        if (result != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row != nullptr && row[0] != nullptr)
            {
                stats["image_analyses"] = atoi(row[0]);
            }
            mysql_free_result(result);
        }
    }

    // 视频分析数量
    if (mysql_query(connection, "SELECT COUNT(*) FROM media_analysis WHERE file_type = 'video'") == 0)
    {
        MYSQL_RES *result = mysql_store_result(connection);
        if (result != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row != nullptr && row[0] != nullptr)
            {
                stats["video_analyses"] = atoi(row[0]);
            }
            mysql_free_result(result);
        }
    }

    // 平均响应时间
    if (mysql_query(connection, "SELECT AVG(response_time) FROM media_analysis") == 0)
    {
        MYSQL_RES *result = mysql_store_result(connection);
        if (result != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row != nullptr && row[0] != nullptr)
            {
                stats["avg_response_time"] = atof(row[0]);
            }
            mysql_free_result(result);
        }
    }

    // 最常用标签
    if (mysql_query(connection, "SELECT tags FROM media_analysis WHERE tags IS NOT NULL AND tags != ''") == 0)
    {
        MYSQL_RES *result = mysql_store_result(connection);
        if (result != nullptr)
        {
            std::map<std::string, int> tag_counts;
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result)) != nullptr)
            {
                if (row[0] != nullptr)
                {
                    std::string tags_str = row[0];
                    // 解析标签字符串，假设格式为 "tag1, tag2, tag3"
                    auto tags = utils::split(tags_str, ',');
                    for (const auto &tag : tags)
                    {
                        std::string clean_tag = utils::trim(tag);
                        if (!clean_tag.empty())
                        {
                            tag_counts[clean_tag]++;
                        }
                    }
                }
            }
            mysql_free_result(result);

            // 转换为数组并排序
            nlohmann::json top_tags = nlohmann::json::array();
            std::vector<std::pair<std::string, int>> sorted_tags(tag_counts.begin(), tag_counts.end());
            std::sort(sorted_tags.begin(), sorted_tags.end(),
                      [](const auto &a, const auto &b)
                      { return a.second > b.second; });

            // 只取前10个标签
            int count = 0;
            for (const auto &pair : sorted_tags)
            {
                if (count >= 10)
                    break;
                nlohmann::json tag_obj;
                tag_obj["tag"] = pair.first;
                tag_obj["count"] = pair.second;
                top_tags.push_back(tag_obj);
                count++;
            }

            stats["top_tags"] = top_tags;
        }
    }

    return stats;
}
