
#ifndef TASK_MANAGER_HPP
#define TASK_MANAGER_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <functional>
#include <future>
#include <atomic>
#include <memory>
#include "DoubaoMediaAnalyzer.hpp"

// 分析任务结构
struct AnalysisTask
{
    std::string id;                                       // 任务唯一ID
    std::string media_url;                                // 媒体URL
    std::string media_type;                               // 媒体类型 ("image" 或 "video")
    std::string prompt;                                   // 分析提示词
    int max_tokens;                                       // 最大令牌数
    int video_frames;                                     // 视频帧数
    bool save_to_db;                                      // 是否保存到数据库
    std::string file_id;                                  // Excel文件中的唯一标识符
    std::function<void(const AnalysisResult &)> callback; // 完成回调
};

// 任务结果结构
struct TaskResult
{
    std::string task_id;   // 任务ID
    bool success;          // 是否成功
    AnalysisResult result; // 分析结果
    std::string error;     // 错误信息
};

// 任务队列管理器
class TaskManager
{
public:
    // 单例模式
    static TaskManager &getInstance();

    // 初始化线程池
    void initialize(size_t thread_count = 4, const std::string &api_key = "");

    // 添加分析任务
    std::future<TaskResult> addTask(const AnalysisTask &task);

    // 批量添加分析任务
    std::vector<std::future<TaskResult>> addTasks(const std::vector<AnalysisTask> &tasks);

    // 关闭线程池
    void shutdown();

    // 获取当前任务数量
    size_t getPendingTaskCount() const;

    // 获取活跃线程数
    size_t getActiveThreadCount() const;

private:
    TaskManager() = default;
    ~TaskManager();

    // 禁用拷贝构造和赋值
    TaskManager(const TaskManager &) = delete;
    TaskManager &operator=(const TaskManager &) = delete;

    // 工作线程函数
    void workerThread();

    // 执行单个任务
    TaskResult executeTask(const AnalysisTask &task);

    // 线程池
    std::vector<std::thread> workers_;

    // 任务队列
    std::queue<AnalysisTask> tasks_;

    // 同步原语
    std::mutex queue_mutex_;
    std::condition_variable condition_;

    // 状态标志
    std::atomic<bool> stop_;
    std::atomic<size_t> active_threads_;

    // 分析器实例
    std::shared_ptr<DoubaoMediaAnalyzer> analyzer_;
};

#endif // TASK_MANAGER_HPP
