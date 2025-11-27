
#ifndef CURL_CONNECTION_POOL_HPP
#define CURL_CONNECTION_POOL_HPP

#include <curl/curl.h>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <string>

// 封装CURL连接的类
class CurlConnection
{
public:
    CurlConnection();
    ~CurlConnection();

    // 禁用拷贝构造和赋值
    CurlConnection(const CurlConnection &) = delete;
    CurlConnection &operator=(const CurlConnection &) = delete;

    // 获取原始CURL指针
    CURL *get() const { return curl_; }

    // 重置连接状态
    void reset();

    // 检查连接是否有效
    bool is_valid() const { return curl_ != nullptr; }

private:
    CURL *curl_;
};

// 连接池类
class CurlConnectionPool
{
public:
    // 获取单例实例
    static CurlConnectionPool &getInstance();

    // 初始化连接池
    void initialize(size_t pool_size = 30);

    // 获取一个连接
    std::shared_ptr<CurlConnection> acquire();

    // 归还一个连接
    void release(std::shared_ptr<CurlConnection> connection);

    // 关闭连接池
    void shutdown();

    // 获取当前活跃连接数
    size_t get_active_connections() const;

    // 获取当前空闲连接数
    size_t get_idle_connections() const;

private:
    CurlConnectionPool() = default;
    ~CurlConnectionPool();

    // 禁用拷贝构造和赋值
    CurlConnectionPool(const CurlConnectionPool &) = delete;
    CurlConnectionPool &operator=(const CurlConnectionPool &) = delete;

    // 创建新连接
    std::shared_ptr<CurlConnection> create_connection();

    // 连接池
    std::queue<std::shared_ptr<CurlConnection>> connections_;

    // 同步原语
    mutable std::mutex mutex_;
    std::condition_variable condition_;

    // 状态标志
    bool shutdown_;
    size_t pool_size_;

    // 统计信息
    size_t active_connections_;
};

#endif // CURL_CONNECTION_POOL_HPP
