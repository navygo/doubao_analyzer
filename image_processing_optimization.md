# 图片处理性能优化方案

## 问题分析

根据日志分析，图片处理过程中存在以下性能瓶颈：

1. Base64编码过程耗时
2. 大图片压缩处理耗时
3. HTTP请求传输耗时
4. 串行处理多个图片，没有利用并发

## 优化方案

### 1. 图片处理优化

#### 1.1 优化Base64编码
- 使用更高效的Base64编码库，如Intel IPP或OpenSSL的Base64函数
- 采用分块编码方式，避免一次性加载大文件到内存
- 实现并行Base64编码，将图片分成多个块并行处理

#### 1.2 图片压缩优化
- 降低图片压缩阈值，从512KB降至256KB
- 进一步减小目标尺寸，从384像素降至256像素
- 使用更快的插值算法，如INTER_NEAREST
- 调整JPEG质量参数，平衡质量和速度

#### 1.3 图片缓存机制
- 实现图片处理结果缓存，避免重复处理相同图片
- 使用文件哈希作为缓存键

### 2. HTTP请求优化

#### 2.1 连接池优化
- 增加连接池大小，从30个连接增至50个
- 优化连接复用策略，减少连接创建和销毁开销
- 实现连接预热机制，提前创建连接

#### 2.2 请求优化
- 启用HTTP/2多路复用
- 实现请求批处理，减少网络往返次数
- 使用更高效的数据压缩算法

### 3. 并发处理优化

#### 3.1 并行图片处理
- 实现多线程图片处理，充分利用多核CPU
- 使用线程池管理并发任务
- 合理设置并发度，避免资源竞争

#### 3.2 异步API请求
- 实现异步HTTP请求，避免阻塞
- 使用回调或Future/Promise模式处理响应
- 实现请求队列和优先级机制

## 具体实现建议

### 1. 修改utils.cpp中的base64_encode_file函数

```cpp
// 使用更高效的Base64编码实现
std::string base64_encode_file_optimized(const std::string &file_path) {
    // 1. 使用内存映射文件，避免一次性加载整个文件
    // 2. 使用OpenSSL的Base64编码函数
    // 3. 实现分块编码，减少内存占用
}
```

### 2. 修改图片压缩逻辑

```cpp
// 优化图片压缩参数
if (file_size > 256 * 1024) { // 降低阈值至256KB
    // 使用更快的插值算法
    cv::resize(img, resized, cv::Size(), scale, scale, cv::INTER_NEAREST);
    // 进一步降低JPEG质量
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 60, cv::IMWRITE_JPEG_OPTIMIZE, 1};
}
```

### 3. 实现并发处理

```cpp
// 在DoubaoMediaAnalyzer类中添加线程池
class DoubaoMediaAnalyzer {
private:
    std::unique_ptr<ThreadPool> thread_pool_;

    // 并发处理图片
    std::vector<AnalysisResult> batch_analyze_concurrent(
        const std::string &media_folder,
        const std::string &prompt,
        int max_files = 5,
        const std::string &file_type = "all");
};
```

### 4. 优化连接池

```cpp
// 增加连接池大小，优化连接复用
void CurlConnectionPool::initialize(size_t pool_size) {
    // 增加连接池大小至50
    pool_size_ = std::max(pool_size, static_cast<size_t>(50));

    // 实现连接预热
    for (size_t i = 0; i < pool_size_; ++i) {
        auto connection = create_connection();
        if (connection && connection->is_valid()) {
            // 预热连接，执行一个简单的请求
            preheat_connection(connection);
            connections_.push(connection);
        }
    }
}
```

## 预期效果

1. 图片处理速度提升30-50%
2. 内存占用减少20-30%
3. 网络传输效率提升20-40%
4. 整体处理时间减少40-60%

## 实施计划

1. 第一阶段：优化图片处理逻辑，包括Base64编码和压缩
2. 第二阶段：实现并发处理机制
3. 第三阶段：优化HTTP请求和连接池

## 注意事项

1. 优化过程中需要确保功能正确性，避免引入新的bug
2. 需要充分测试，特别是在高并发场景下
3. 注意资源使用情况，避免过度消耗CPU和内存
