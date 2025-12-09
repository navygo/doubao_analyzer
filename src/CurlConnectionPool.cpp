
#include "CurlConnectionPool.hpp"
#include "utils.hpp"
#include <iostream>
#include <chrono>

// CurlConnection 实现
CurlConnection::CurlConnection() : curl_(nullptr)
{
    curl_ = curl_easy_init();
    if (curl_)
    {
        // 设置全局选项
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPIDLE, 60L);
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPINTVL, 30L);
        curl_easy_setopt(curl_, CURLOPT_FORBID_REUSE, 0L);

        // 添加更多性能优化选项
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);                                      // 避免信号中断
        curl_easy_setopt(curl_, CURLOPT_TCP_NODELAY, 1L);                                   // 禁用Nagle算法，减少延迟
        curl_easy_setopt(curl_, CURLOPT_BUFFERSIZE, 204800L);                               // 增大缓冲区大小到200KB
        curl_easy_setopt(curl_, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br");              // 启用压缩，包括Brotli
        curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE); // 默认启用HTTP/2

        // 设置默认超时
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 10L);
    }
}

CurlConnection::~CurlConnection()
{
    if (curl_)
    {
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
    }
}

void CurlConnection::reset()
{
    if (curl_)
    {
        curl_easy_reset(curl_);

        // 重新设置全局选项
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPIDLE, 60L);
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPINTVL, 30L);
        curl_easy_setopt(curl_, CURLOPT_FORBID_REUSE, 0L);
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl_, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(curl_, CURLOPT_BUFFERSIZE, 102400L);
        curl_easy_setopt(curl_, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 10L);
    }
}

// CurlConnectionPool 实现
CurlConnectionPool &CurlConnectionPool::getInstance()
{
    static CurlConnectionPool instance;
    return instance;
}

CurlConnectionPool::~CurlConnectionPool()
{
    shutdown();
}

void CurlConnectionPool::initialize(size_t pool_size)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!connections_.empty())
    {
        return; // 已经初始化
    }

    shutdown_ = false;
    // 增加连接池大小，提高并发处理能力
    pool_size_ = std::max(pool_size, static_cast<size_t>(20));
    active_connections_ = 0;

    std::cout << "🔧 [连接池] 初始化连接池，目标连接数: " << pool_size_ << std::endl;

    // 创建初始连接
    for (size_t i = 0; i < pool_size_; ++i)
    {
        auto connection = create_connection();
        if (connection && connection->is_valid())
        {
            // 预热连接，发送一个简单的HEAD请求
            preheat_connection(connection);
            connections_.push(connection);
        }
    }

    std::cout << "✅ CURL连接池已初始化，连接数: " << connections_.size() << std::endl;
}

std::shared_ptr<CurlConnection> CurlConnectionPool::acquire()
{
    std::unique_lock<std::mutex> lock(mutex_);

    // 等待直到有可用连接或关闭
    condition_.wait(lock, [this]
                    { return !connections_.empty() || shutdown_; });

    if (shutdown_)
    {
        return nullptr;
    }

    // 获取连接
    auto connection = connections_.front();
    connections_.pop();
    active_connections_++;

    // 重置连接状态
    connection->reset();

    return connection;
}

void CurlConnectionPool::release(std::shared_ptr<CurlConnection> connection)
{
    if (!connection || !connection->is_valid())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (shutdown_)
    {
        return; // 连接池已关闭，不接收连接
    }

    // 归还连接
    connections_.push(connection);
    active_connections_--;

    // 通知等待的线程
    condition_.notify_one();
}

void CurlConnectionPool::shutdown()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (shutdown_)
    {
        return; // 已经关闭
    }

    shutdown_ = true;

    // 清空连接池
    while (!connections_.empty())
    {
        connections_.pop();
    }

    active_connections_ = 0;

    // 通知所有等待的线程
    condition_.notify_all();

    std::cout << "✅ CURL连接池已关闭" << std::endl;
}

size_t CurlConnectionPool::get_active_connections() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return active_connections_;
}

size_t CurlConnectionPool::get_idle_connections() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return connections_.size();
}

std::shared_ptr<CurlConnection> CurlConnectionPool::create_connection()
{
    auto connection = std::make_shared<CurlConnection>();

    if (!connection->is_valid())
    {
        std::cerr << "❌ 创建CURL连接失败" << std::endl;
        return nullptr;
    }

    return connection;
}

// 预热连接，发送一个简单的HEAD请求
void CurlConnectionPool::preheat_connection(std::shared_ptr<CurlConnection> connection)
{
    if (!connection || !connection->is_valid())
    {
        return;
    }

    CURL *curl = connection->get();
    if (!curl)
    {
        return;
    }

    // 设置一个简单的HEAD请求到常见的服务器
    curl_easy_setopt(curl, CURLOPT_URL, "http://httpbin.org/head");
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);         // HEAD请求
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);        // 短超时
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // 允许重定向

    // 执行请求但不处理响应
    curl_easy_perform(curl);

    // 重置连接状态，以便后续使用
    connection->reset();
}
