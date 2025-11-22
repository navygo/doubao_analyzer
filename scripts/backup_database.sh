#!/bin/bash

# 数据库备份脚本
# 用法: ./backup_database.sh [备份路径]

# 默认配置
DB_HOST="localhost"
DB_USER="root"
DB_PASS="password"
DB_NAME="doubao_analyzer"

# 从参数获取备份路径
BACKUP_PATH=$1
if [ -z "$BACKUP_PATH" ]; then
    BACKUP_PATH="./doubao_analyzer_backup_$(date +%Y%m%d_%H%M%S).sql"
fi

echo "开始备份数据库..."
echo "数据库: $DB_NAME"
echo "备份路径: $BACKUP_PATH"

# 创建备份目录
mkdir -p "$(dirname "$BACKUP_PATH")"

# 执行备份
mysqldump -h $DB_HOST -u $DB_USER -p$DB_PASS $DB_NAME > $BACKUP_PATH

if [ $? -eq 0 ]; then
    echo "✅ 数据库备份成功: $BACKUP_PATH"

    # 压缩备份文件
    gzip $BACKUP_PATH
    echo "✅ 备份文件已压缩: ${BACKUP_PATH}.gz"
else
    echo "❌ 数据库备份失败"
    exit 1
fi
