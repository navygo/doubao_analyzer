
#include "CurlConnectionPool.hpp"
#include "utils.hpp"
#include <iostream>
#include <chrono>

// CurlConnection 实现
CurlConnection::CurlConnection() : curl_(nullptr) {
    curl_ = curl_easy_init();
    if (curl_) {
        // 设置全局选项
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPIDLE, 60L);
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPINTVL, 30L);
        curl_easy_setopt(curl_, CURLOPT_FORBID_REUSE, 0L);

        // 添加更多性能优化选项
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);        // 避免信号中断
        curl_easy_setopt(curl_, CURLOPT_TCP_NODELAY, 1L);     // 禁用Nagle算法，减少延迟
        curl_easy_setopt(curl_, CURLOPT_BUFFERSIZE, 102400L); // 增大缓冲区大小到100KB
        curl_easy_setopt(curl_, CURLOPT_ACCEPT_ENCODING, "gzip, deflate"); // 启用压缩

        // 设置默认超时
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 10L);
    }
}

CurlConnection::~CurlConnection() {
    if (curl_) {
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
    }
}

void CurlConnection::reset() {
    if (curl_) {
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
CurlConnectionPool& CurlConnectionPool::getInstance() {
    static CurlConnectionPool instance;
    return instance;
}

CurlConnectionPool::~CurlConnectionPool() {
    shutdown();
}

void CurlConnectionPool::initialize(size_t pool_size) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!connections_.empty()) {
        return; // 已经初始化
    }

    shutdown_ = false;
    pool_size_ = pool_size;
    active_connections_ = 0;

    // 创建初始连接
    for (size_t i = 0; i < pool_size_; ++i) {
        auto connection = create_connection();
        if (connection && connection->is_valid()) {
            connections_.push(connection);
        }
    }

    std::cout << "✅ CURL连接池已初始化，连接数: " << connections_.size() << std::endl;
}

std::shared_ptr<CurlConnection> CurlConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);

    // 等待直到有可用连接或关闭
    condition_.wait(lock, [this] { return !connections_.empty() || shutdown_; });

    if (shutdown_) {
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

void CurlConnectionPool::release(std::shared_ptr<CurlConnection> connection) {
    if (!connection || !connection->is_valid()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (shutdown_) {
        return; // 连接池已关闭，不接收连接
    }

    // 归还连接
    connections_.push(connection);
    active_connections_--;

    // 通知等待的线程
    condition_.notify_one();
}

void CurlConnectionPool::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (shutdown_) {
        return; // 已经关闭
    }

    shutdown_ = true;

    // 清空连接池
    while (!connections_.empty()) {
        connections_.pop();
    }

    active_connections_ = 0;

    // 通知所有等待的线程
    condition_.notify_all();

    std::cout << "✅ CURL连接池已关闭" << std::endl;
}

size_t CurlConnectionPool::get_active_connections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_connections_;
}

size_t CurlConnectionPool::get_idle_connections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connections_.size();
}

std::shared_ptr<CurlConnection> CurlConnectionPool::create_connection() {
    auto connection = std::make_shared<CurlConnection>();

    if (!connection->is_valid()) {
        std::cerr << "❌ 创建CURL连接失败" << std::endl;
        return nullptr;
    }

    return connection;
}
