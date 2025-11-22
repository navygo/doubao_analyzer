#!/bin/bash

# 数据库恢复脚本
# 用法: ./restore_database.sh [备份文件路径]

# 默认配置
DB_HOST="localhost"
DB_USER="root"
DB_PASS="password"
DB_NAME="doubao_analyzer"

# 从参数获取备份文件路径
BACKUP_FILE=$1
if [ -z "$BACKUP_FILE" ]; then
    echo "❌ 请提供备份文件路径"
    echo "用法: ./restore_database.sh [备份文件路径]"
    exit 1
fi

# 检查文件是否存在
if [ ! -f "$BACKUP_FILE" ]; then
    echo "❌ 备份文件不存在: $BACKUP_FILE"
    exit 1
fi

echo "开始恢复数据库..."
echo "数据库: $DB_NAME"
echo "备份文件: $BACKUP_FILE"

# 如果是压缩文件，先解压
if [[ $BACKUP_FILE == *.gz ]]; then
    echo "正在解压备份文件..."
    gunzip -c $BACKUP_FILE > /tmp/restore_temp.sql
    RESTORE_FILE="/tmp/restore_temp.sql"
else
    RESTORE_FILE=$BACKUP_FILE
fi

# 确认恢复操作
read -p "⚠️  警告: 此操作将覆盖现有数据库。是否继续? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "❌ 恢复操作已取消"
    exit 1
fi

# 执行恢复
mysql -h $DB_HOST -u $DB_USER -p$DB_PASS $DB_NAME < $RESTORE_FILE

if [ $? -eq 0 ]; then
    echo "✅ 数据库恢复成功"

    # 清理临时文件
    if [ -f "/tmp/restore_temp.sql" ]; then
        rm /tmp/restore_temp.sql
    fi
else
    echo "❌ 数据库恢复失败"
    exit 1
fi
