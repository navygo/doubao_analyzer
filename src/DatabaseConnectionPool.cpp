#include "DatabaseConnectionPool.hpp"
#include <iostream>
#include <thread>
#include <algorithm>
#include <sstream>
#include "ConfigManager.hpp"

// ConnectionWrapper 实现
ConnectionWrapper::ConnectionWrapper(MYSQL *conn, DatabaseConnectionPool *pool)
    : connection_(conn), pool_(pool)
{
}

ConnectionWrapper::~ConnectionWrapper()
{
    if (connection_ && pool_)
    {
        pool_->return_connection(connection_);
    }
}

ConnectionWrapper::ConnectionWrapper(ConnectionWrapper &&other) noexcept
    : connection_(other.connection_), pool_(other.pool_)
{
    other.connection_ = nullptr;
    other.pool_ = nullptr;
}

ConnectionWrapper &ConnectionWrapper::operator=(ConnectionWrapper &&other) noexcept
{
    if (this != &other)
    {
        // 释放当前资源
        if (connection_ && pool_)
        {
            pool_->return_connection(connection_);
        }

        // 移动资源
        connection_ = other.connection_;
        pool_ = other.pool_;

        // 清空源对象
        other.connection_ = nullptr;
        other.pool_ = nullptr;
    }
    return *this;
}

bool ConnectionWrapper::is_valid() const
{
    return connection_ && pool_ && pool_->is_connection_valid(connection_);
}

bool ConnectionWrapper::reset()
{
    if (!connection_ || !pool_)
    {
        return false;
    }

    return pool_->is_connection_valid(connection_);
}

// DatabaseConnectionPool 实现
DatabaseConnectionPool::DatabaseConnectionPool(const DatabaseConfig &db_config, const PoolConfig &pool_config)
    : db_config_(db_config), pool_config_(pool_config), initialized_(false), shutdown_requested_(false), active_connections_(0)
{
}

DatabaseConnectionPool::~DatabaseConnectionPool()
{
    shutdown();
}

bool DatabaseConnectionPool::initialize()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_)
    {
        return true;
    }

    // 创建初始连接
    for (size_t i = 0; i < pool_config_.initial_size; ++i)
    {
        MYSQL *conn = create_connection();
        if (conn)
        {
            available_connections_.push(conn);
            all_connections_.push_back(conn);

            // 记录连接信息
            ConnectionInfo info;
            info.connection = conn;
            info.last_used = std::chrono::steady_clock::now();
            connection_infos_.push_back(info);
        }
        else
        {
            std::cerr << "Failed to create initial database connection " << i + 1 << std::endl;
        }
    }

    if (available_connections_.empty())
    {
        std::cerr << "Failed to create any database connections" << std::endl;
        return false;
    }

    initialized_ = true;

    // 启动清理线程
    cleanup_thread_ = std::thread(&DatabaseConnectionPool::cleanup_worker, this);

    std::cout << "Database connection pool initialized with " << available_connections_.size()
              << " connections" << std::endl;

    return true;
}

ConnectionWrapper DatabaseConnectionPool::get_connection()
{
    if (!initialized_)
    {
        std::cerr << "Connection pool not initialized" << std::endl;
        return ConnectionWrapper(nullptr, this);
    }

    std::unique_lock<std::mutex> lock(mutex_);

    // 等待可用连接
    auto timeout = std::chrono::milliseconds(pool_config_.wait_timeout);
    if (!condition_.wait_for(lock, timeout, [this]
                             { return !available_connections_.empty() || shutdown_requested_; }))
    {
        std::cerr << "Timeout waiting for database connection" << std::endl;
        return ConnectionWrapper(nullptr, this);
    }

    if (shutdown_requested_)
    {
        return ConnectionWrapper(nullptr, this);
    }

    // 获取连接
    MYSQL *connection = available_connections_.front();
    available_connections_.pop();

    // 更新连接使用时间
    auto it = std::find_if(connection_infos_.begin(), connection_infos_.end(),
                           [connection](const ConnectionInfo &info)
                           { return info.connection == connection; });

    if (it != connection_infos_.end())
    {
        it->last_used = std::chrono::steady_clock::now();
    }

    // 检查连接有效性
    if (!is_connection_valid(connection))
    {
        // 连接无效，尝试重新创建
        destroy_connection(connection);
        connection = create_connection();

        if (!connection)
        {
            std::cerr << "Failed to recreate database connection" << std::endl;
            return ConnectionWrapper(nullptr, this);
        }

        // 更新连接信息
        if (it != connection_infos_.end())
        {
            it->connection = connection;
            it->last_used = std::chrono::steady_clock::now();
        }

        // 更新连接列表
        auto conn_it = std::find(all_connections_.begin(), all_connections_.end(), connection);
        if (conn_it != all_connections_.end())
        {
            *conn_it = connection;
        }
    }

    active_connections_++;

    return ConnectionWrapper(connection, this);
}

void DatabaseConnectionPool::return_connection(MYSQL *connection)
{
    if (!connection)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (shutdown_requested_)
    {
        // 如果正在关闭，直接销毁连接
        destroy_connection(connection);
        return;
    }

    // 将连接放回队列
    available_connections_.push(connection);
    active_connections_--;

    // 通知等待的线程
    condition_.notify_one();
}

void DatabaseConnectionPool::shutdown()
{
    if (!initialized_)
    {
        return;
    }

    // 请求关闭
    shutdown_requested_ = true;
    condition_.notify_all();

    // 等待清理线程结束
    if (cleanup_thread_.joinable())
    {
        cleanup_thread_.join();
    }

    // 销毁所有连接
    std::lock_guard<std::mutex> lock(mutex_);

    // 清空队列
    while (!available_connections_.empty())
    {
        MYSQL *conn = available_connections_.front();
        available_connections_.pop();
        destroy_connection(conn);
    }

    // 销毁所有连接
    for (MYSQL *conn : all_connections_)
    {
        destroy_connection(conn);
    }

    all_connections_.clear();
    connection_infos_.clear();

    initialized_ = false;

    std::cout << "Database connection pool shutdown" << std::endl;
}

std::string DatabaseConnectionPool::get_status() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::stringstream ss;
    ss << "Connection Pool Status:" << std::endl;
    ss << "  Total connections: " << all_connections_.size() << std::endl;
    ss << "  Available connections: " << available_connections_.size() << std::endl;
    ss << "  Active connections: " << active_connections_ << std::endl;
    ss << "  Max connections: " << pool_config_.max_size << std::endl;

    return ss.str();
}

void DatabaseConnectionPool::cleanup_idle_connections()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (shutdown_requested_ || !initialized_)
    {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto idle_threshold = std::chrono::seconds(pool_config_.max_idle_time);

    // 找出长时间未使用的连接
    for (auto it = connection_infos_.begin(); it != connection_infos_.end();)
    {
        auto idle_time = now - it->last_used;

        if (idle_time > idle_threshold && all_connections_.size() > pool_config_.initial_size)
        {
            // 检查连接是否在可用队列中
            bool in_queue = false;
            std::queue<MYSQL *> temp_queue;

            while (!available_connections_.empty())
            {
                MYSQL *conn = available_connections_.front();
                available_connections_.pop();

                if (conn == it->connection)
                {
                    in_queue = true;
                    destroy_connection(conn);

                    // 从所有连接列表中移除
                    auto conn_it = std::find(all_connections_.begin(), all_connections_.end(), conn);
                    if (conn_it != all_connections_.end())
                    {
                        all_connections_.erase(conn_it);
                    }
                }
                else
                {
                    temp_queue.push(conn);
                }
            }

            // 重新构建队列
            available_connections_ = std::move(temp_queue);

            if (!in_queue)
            {
                // 连接正在使用中，跳过
                ++it;
            }
            else
            {
                // 从信息列表中移除
                it = connection_infos_.erase(it);
            }
        }
        else
        {
            ++it;
        }
    }

    // 如果可用连接太少，创建新连接
    if (available_connections_.size() < pool_config_.initial_size &&
        all_connections_.size() < pool_config_.max_size)
    {

        size_t to_create = std::min(pool_config_.initial_size - available_connections_.size(),
                                    pool_config_.max_size - all_connections_.size());

        for (size_t i = 0; i < to_create; ++i)
        {
            MYSQL *conn = create_connection();
            if (conn)
            {
                available_connections_.push(conn);
                all_connections_.push_back(conn);

                // 记录连接信息
                ConnectionInfo info;
                info.connection = conn;
                info.last_used = std::chrono::steady_clock::now();
                connection_infos_.push_back(info);
            }
        }
    }
}

void DatabaseConnectionPool::cleanup_worker()
{
    while (!shutdown_requested_)
    {
        // 每分钟清理一次
        std::this_thread::sleep_for(std::chrono::minutes(1));

        if (!shutdown_requested_)
        {
            cleanup_idle_connections();
        }
    }
}

MYSQL *DatabaseConnectionPool::create_connection()
{
    MYSQL *conn = mysql_init(nullptr);
    if (!conn)
    {
        std::cerr << "mysql_init() failed" << std::endl;
        return nullptr;
    }

    // 设置连接选项
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &db_config_.connection_timeout);
    mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &db_config_.read_timeout);
    mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &db_config_.write_timeout);

    // MYSQL_OPT_RECONNECT 已被弃用，不再使用
    // 如果需要自动重连功能，应该在应用层实现

    // 连接到数据库
    if (mysql_real_connect(conn, db_config_.host.c_str(), db_config_.user.c_str(),
                           db_config_.password.c_str(), db_config_.database.c_str(),
                           db_config_.port, nullptr, CLIENT_MULTI_STATEMENTS) == nullptr)
    {
        std::cerr << "mysql_real_connect() failed: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return nullptr;
    }

    // 设置字符集
    if (mysql_set_character_set(conn, db_config_.charset.c_str()))
    {
        std::cerr << "Error setting character set: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return nullptr;
    }

    return conn;
}

void DatabaseConnectionPool::destroy_connection(MYSQL *connection)
{
    if (connection)
    {
        mysql_close(connection);
    }
}

bool DatabaseConnectionPool::is_connection_valid(MYSQL *connection)
{
    if (!connection)
    {
        return false;
    }

    // 使用 ping 检查连接
    return mysql_ping(connection) == 0;
}
