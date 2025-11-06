-- 豆包媒体分析工具数据库初始化脚本

-- 创建数据库（如果不存在）
CREATE DATABASE IF NOT EXISTS doubao_analyzer 
CHARACTER SET utf8mb4 
COLLATE utf8mb4_unicode_ci;

-- 使用数据库
USE doubao_analyzer;

-- 创建媒体分析结果表
CREATE TABLE IF NOT EXISTS media_analysis (
    id INT AUTO_INCREMENT PRIMARY KEY,
    file_path VARCHAR(1024) NOT NULL,
    file_name VARCHAR(255) NOT NULL,
    file_type ENUM('image', 'video') NOT NULL,
    analysis_result TEXT,
    tags VARCHAR(1024),
    response_time DOUBLE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_file_type (file_type),
    INDEX idx_created_at (created_at),
    INDEX idx_tags (tags(255))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 创建用户表（用于管理API密钥和用户设置）
CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    api_key VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_username (username)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 创建分析历史表（用于记录用户操作历史）
CREATE TABLE IF NOT EXISTS analysis_history (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,
    operation_type VARCHAR(50) NOT NULL,
    operation_details TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_operation_type (operation_type),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 插入默认管理员用户（密码需要通过应用程序设置）
INSERT IGNORE INTO users (username, api_key) VALUES ('admin', 'YOUR_API_KEY_HERE');

-- 创建数据库备份存储过程
DELIMITER //
CREATE PROCEDURE IF NOT EXISTS backup_database(IN backup_path VARCHAR(255))
BEGIN
    SET @sql = CONCAT('mysqldump -u root -p doubao_analyzer > ', backup_path);
    PREPARE stmt FROM @sql;
    EXECUTE stmt;
    DEALLOCATE PREPARE stmt;
END //
DELIMITER ;

-- 创建清理旧记录的存储过程（保留最近N天的记录）
DELIMITER //
CREATE PROCEDURE IF NOT EXISTS cleanup_old_records(IN days_to_keep INT)
BEGIN
    DELETE FROM media_analysis 
    WHERE created_at < DATE_SUB(NOW(), INTERVAL days_to_keep DAY);
END //
DELIMITER ;

-- 创建按标签统计的视图
CREATE OR REPLACE VIEW tag_statistics AS
SELECT 
    SUBSTRING_INDEX(SUBSTRING_INDEX(tags, ',', n), ',', -1) AS tag,
    COUNT(*) AS count
FROM media_analysis
CROSS JOIN (
    SELECT 1 AS n UNION SELECT 2 UNION SELECT 3 UNION SELECT 4 UNION SELECT 5
    UNION SELECT 6 UNION SELECT 7 UNION SELECT 8 UNION SELECT 9 UNION SELECT 10
) numbers
WHERE tags IS NOT NULL AND tags != ''
AND n <= LENGTH(tags) - LENGTH(REPLACE(tags, ',', '')) + 1
GROUP BY tag
ORDER BY count DESC;
