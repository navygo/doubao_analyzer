#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <mysql/mysql.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <sstream>
#include "ConfigManager.hpp"

// 连接包装类，用于自动归还连接
class ConnectionWrapper
{
public:
    ConnectionWrapper(MYSQL *conn, class DatabaseConnectionPool *pool);
    ~ConnectionWrapper();

    // 禁止拷贝
    ConnectionWrapper(const ConnectionWrapper &) = delete;
    ConnectionWrapper &operator=(const ConnectionWrapper &) = delete;

    // 允许移动
    ConnectionWrapper(ConnectionWrapper &&other) noexcept;
    ConnectionWrapper &operator=(ConnectionWrapper &&other) noexcept;

    // 获取原始连接
    MYSQL *get() const { return connection_; }

    // 检查连接是否有效
    bool is_valid() const;

    // 重置连接
    bool reset();

private:
    MYSQL *connection_;
    DatabaseConnectionPool *pool_;
};

// 数据库连接池类
class DatabaseConnectionPool
{
    // 声明 ConnectionWrapper 为友元类，以便访问私有方法
    friend class ConnectionWrapper;

public:
    // 构造函数
    DatabaseConnectionPool(const DatabaseConfig &db_config, const PoolConfig &pool_config);

    // 析构函数
    ~DatabaseConnectionPool();

    // 初始化连接池
    bool initialize();

    // 获取一个连接
    ConnectionWrapper get_connection();

    // 归还连接
    void return_connection(MYSQL *connection);

    // 关闭连接池
    void shutdown();

    // 获取连接池状态信息
    std::string get_status() const;

    // 清理空闲连接
    void cleanup_idle_connections();

    // 检查连接池是否有效
    bool is_valid() const { return initialized_; }

    // 检查连接是否有效
    bool is_connection_valid(MYSQL *connection);

private:
    // 创建新连接
    MYSQL *create_connection();

    // 销毁连接
    void destroy_connection(MYSQL *connection);

    // 清理连接池的工作线程
    void cleanup_worker();

    DatabaseConfig db_config_;
    PoolConfig pool_config_;
    std::queue<MYSQL *> available_connections_;
    std::vector<MYSQL *> all_connections_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> initialized_;
    std::atomic<bool> shutdown_requested_;
    std::atomic<size_t> active_connections_;

    // 用于记录连接的最后使用时间
    struct ConnectionInfo
    {
        MYSQL *connection;
        std::chrono::steady_clock::time_point last_used;
    };
    std::vector<ConnectionInfo> connection_infos_;

    // 清理线程
    std::thread cleanup_thread_;
};
