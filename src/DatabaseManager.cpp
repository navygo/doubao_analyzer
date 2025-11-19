#include "DatabaseManager.hpp"
#include "utils.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

DatabaseManager::DatabaseManager(const std::string &host, const std::string &user,
                                 const std::string &password, const std::string &database,
                                 unsigned int port, const std::string &charset,
                                 int connection_timeout, int read_timeout, int write_timeout)
    : connection_(nullptr), host_(host), user_(user), password_(password),
      database_(database), port_(port), charset_(charset),
      connection_timeout_(connection_timeout), read_timeout_(read_timeout), write_timeout_(write_timeout)
{
}

DatabaseManager::DatabaseManager(const DatabaseConfig &config)
    : connection_(nullptr), host_(config.host), user_(config.user), password_(config.password),
      database_(config.database), port_(config.port), charset_(config.charset),
      connection_timeout_(config.connection_timeout), read_timeout_(config.read_timeout),
      write_timeout_(config.write_timeout)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::initialize()
{
    if (!connect())
    {
        std::cerr << "Failed to connect to database" << std::endl;
        return false;
    }

    if (!initialize_tables())
    {
        std::cerr << "Failed to initialize database tables" << std::endl;
        return false;
    }

    return true;
}

bool DatabaseManager::connect()
{
    connection_ = mysql_init(nullptr);
    if (connection_ == nullptr)
    {
        std::cerr << "mysql_init() failed" << std::endl;
        return false;
    }

    if (mysql_real_connect(connection_, host_.c_str(), user_.c_str(),
                           password_.c_str(), database_.c_str(), port_,
                           nullptr, CLIENT_MULTI_STATEMENTS) == nullptr)
    {
        std::cerr << "mysql_real_connect() failed: " << mysql_error(connection_) << std::endl;
        mysql_close(connection_);
        connection_ = nullptr;
        return false;
    }

    // 设置字符集
    if (mysql_set_character_set(connection_, charset_.c_str()))
    {
        std::cerr << "Error setting character set: " << mysql_error(connection_) << std::endl;
        mysql_close(connection_);
        connection_ = nullptr;
        return false;
    }

    // 设置超时选项
    mysql_options(connection_, MYSQL_OPT_CONNECT_TIMEOUT, &connection_timeout_);
    mysql_options(connection_, MYSQL_OPT_READ_TIMEOUT, &read_timeout_);
    mysql_options(connection_, MYSQL_OPT_WRITE_TIMEOUT, &write_timeout_);

    return true;
}

void DatabaseManager::close()
{
    if (connection_ != nullptr)
    {
        mysql_close(connection_);
        connection_ = nullptr;
    }
}

bool DatabaseManager::execute_query(const std::string &query)
{
    if (connection_ == nullptr)
    {
        std::cerr << "Database connection is not established" << std::endl;
        return false;
    }

    if (mysql_query(connection_, query.c_str()) != 0)
    {
        std::cerr << "MySQL query error: " << mysql_error(connection_) << std::endl;
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
    if (connection_ == nullptr)
        return false;
    std::string q = "SELECT user_id, expires_at FROM refresh_tokens WHERE token_hash = '" + utils::replace_all(token_hash, "'", "''") + "' LIMIT 1";
    if (!execute_query(q))
        return false;

    MYSQL_RES *result = mysql_store_result(connection_);
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

    // 开始事务
    if (!execute_query("START TRANSACTION"))
    {
        return false;
    }

    try
    {
        for (const auto &record : records)
        {
            if (!save_analysis_result(record))
            {
                throw std::runtime_error("Failed to save a record");
            }
        }

        // 提交事务
        return execute_query("COMMIT");
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in batch save: " << e.what() << std::endl;
        // 回滚事务
        execute_query("ROLLBACK");
        return false;
    }
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

    if (!execute_query(query))
    {
        return results;
    }

    MYSQL_RES *result = mysql_store_result(connection_);
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

    if (!execute_query(query.str()))
    {
        return results;
    }

    MYSQL_RES *result = mysql_store_result(connection_);
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

    // 总分析数量
    if (execute_query("SELECT COUNT(*) FROM media_analysis"))
    {
        MYSQL_RES *result = mysql_store_result(connection_);
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
    if (execute_query("SELECT COUNT(*) FROM media_analysis WHERE file_type = 'image'"))
    {
        MYSQL_RES *result = mysql_store_result(connection_);
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
    if (execute_query("SELECT COUNT(*) FROM media_analysis WHERE file_type = 'video'"))
    {
        MYSQL_RES *result = mysql_store_result(connection_);
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
    if (execute_query("SELECT AVG(response_time) FROM media_analysis"))
    {
        MYSQL_RES *result = mysql_store_result(connection_);
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
    if (execute_query("SELECT tags FROM media_analysis WHERE tags IS NOT NULL AND tags != ''"))
    {
        MYSQL_RES *result = mysql_store_result(connection_);
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
