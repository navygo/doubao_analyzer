#include "DatabaseManager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>

// 根据时间范围查询
std::vector<MediaAnalysisRecord> DatabaseManager::query_by_date_range(const std::string& start_date, const std::string& end_date) {
    std::vector<MediaAnalysisRecord> results;

    std::string query = "SELECT id, file_path, file_name, file_type, analysis_result, tags, response_time, created_at FROM media_analysis";
    query += " WHERE created_at BETWEEN '" + start_date + "' AND '" + end_date + "'";
    query += " ORDER BY created_at DESC";

    if (!execute_query(query)) {
        return results;
    }

    MYSQL_RES* result = mysql_store_result(connection_);
    if (result == nullptr) {
        return results;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        MediaAnalysisRecord record;
        record.id = atoi(row[0]);
        record.file_path = row[1] ? row[1] : "";
        record.file_name = row[2] ? row[2] : "";
        record.file_type = row[3] ? row[3] : "";
        record.analysis_result = row[4] ? row[4] : "";
        record.tags = row[5] ? row[5] : "";
        record.response_time = row[6] ? atof(row[6]) : 0.0;
        record.created_at = row[7] ? row[7] : "";

        results.push_back(record);
    }

    mysql_free_result(result);
    return results;
}

// 备份数据库
bool DatabaseManager::backup_database(const std::string& backup_path) {
    try {
        // 构建备份命令
        std::stringstream cmd;
        cmd << "mysqldump -h " << host_ 
            << " -u " << user_ 
            << " -p" << password_ 
            << " " << database_ 
            << " > " << backup_path;

        // 执行备份命令
        int result = system(cmd.str().c_str());

        if (result == 0) {
            std::cout << "✅ 数据库备份成功: " << backup_path << std::endl;

            // 压缩备份文件
            std::stringstream compress_cmd;
            compress_cmd << "gzip " << backup_path;
            system(compress_cmd.str().c_str());

            std::cout << "✅ 备份文件已压缩: " << backup_path << ".gz" << std::endl;
            return true;
        } else {
            std::cerr << "❌ 数据库备份失败，错误代码: " << result << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "备份数据库异常: " << e.what() << std::endl;
        return false;
    }
}

// 清理旧记录
bool DatabaseManager::cleanup_old_records(int days_to_keep) {
    try {
        // 构建清理SQL
        std::stringstream query;
        query << "DELETE FROM media_analysis "
              << "WHERE created_at < DATE_SUB(NOW(), INTERVAL " << days_to_keep << " DAY)";

        if (!execute_query(query.str())) {
            return false;
        }

        // 获取影响的行数
        my_ulonglong affected_rows = mysql_affected_rows(connection_);
        std::cout << "✅ 已清理 " << affected_rows << " 条旧记录" << std::endl;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "清理旧记录异常: " << e.what() << std::endl;
        return false;
    }
}
