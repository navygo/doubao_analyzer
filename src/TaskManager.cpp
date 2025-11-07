
#include "TaskManager.hpp"
#include "utils.hpp"
#include <iostream>
#include <chrono>

// 单例实现
TaskManager& TaskManager::getInstance() {
    static TaskManager instance;
    return instance;
}

TaskManager::~TaskManager() {
    shutdown();
}

void TaskManager::initialize(size_t thread_count, const std::string& api_key) {
    if (!workers_.empty()) {
        return; // 已经初始化
    }

    stop_ = false;
    active_threads_ = 0;

    // 创建分析器实例
    analyzer_ = std::make_shared<DoubaoMediaAnalyzer>(api_key);

    // 创建工作线程
    for (size_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back(&TaskManager::workerThread, this);
    }

    std::cout << "✅ 任务管理器已初始化，线程数: " << thread_count << std::endl;
}

std::future<TaskResult> TaskManager::addTask(const AnalysisTask& task) {
    auto promise = std::make_shared<std::promise<TaskResult>>();
    auto future = promise->get_future();

    // 创建带回调的任务副本
    AnalysisTask task_with_callback = task;
    task_with_callback.callback = [promise, task](const AnalysisResult& result) {
        TaskResult task_result;
        task_result.task_id = task.id;
        task_result.success = result.success;
        task_result.result = result;
        task_result.error = result.error;

        promise->set_value(task_result);
    };

    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        tasks_.push(task_with_callback);
    }

    condition_.notify_one();
    return future;
}

std::vector<std::future<TaskResult>> TaskManager::addTasks(const std::vector<AnalysisTask>& tasks) {
    std::vector<std::future<TaskResult>> futures;
    futures.reserve(tasks.size());

    for (const auto& task : tasks) {
        futures.push_back(addTask(task));
    }

    return futures;
}

void TaskManager::shutdown() {
    if (stop_) {
        return; // 已经关闭
    }

    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }

    condition_.notify_all();

    // 等待所有线程完成
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
    std::cout << "✅ 任务管理器已关闭" << std::endl;
}

size_t TaskManager::getPendingTaskCount() const {
    std::unique_lock<std::mutex> lock(const_cast<std::mutex&>(queue_mutex_));
    return tasks_.size();
}

size_t TaskManager::getActiveThreadCount() const {
    return active_threads_;
}

void TaskManager::workerThread() {
    while (true) {
        AnalysisTask task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            condition_.wait(lock, [this] {
                return stop_ || !tasks_.empty();
            });

            if (stop_ && tasks_.empty()) {
                return;
            }

            task = tasks_.front();
            tasks_.pop();
        }

        // 执行任务
        active_threads_++;
        TaskResult result = executeTask(task);
        active_threads_--;

        // 调用回调
        if (task.callback) {
            task.callback(result.result);
        }
    }
}

TaskResult TaskManager::executeTask(const AnalysisTask& task) {
    TaskResult result;
    result.task_id = task.id;

    try {
        std::cout << "🔄 开始处理任务: " << task.id << " (" << task.media_type << ")" << std::endl;

        // 根据媒体类型选择分析方法
        if (task.media_type == "image") {
            result.result = analyzer_->analyze_single_image(
                task.media_url, 
                task.prompt, 
                task.max_tokens);
        } 
        else if (task.media_type == "video") {
            result.result = analyzer_->analyze_video_efficiently(
                task.media_url, 
                task.prompt, 
                task.max_tokens,
                "keyframes"); // 使用关键帧提取方法
        }
        else {
            result.result.success = false;
            result.result.error = "不支持的媒体类型: " + task.media_type;
        }

        // 异步保存到数据库
        if (task.save_to_db && result.result.success) {
            // 使用线程池中的线程异步保存，避免阻塞
            std::thread([this, result]() {
                try {
                    analyzer_->save_result_to_database(result.result);
                    std::cout << "✅ 任务结果已保存到数据库: " << result.task_id << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "❌ 保存到数据库失败: " << e.what() << std::endl;
                }
            }).detach();
        }

        result.success = result.result.success;
        if (!result.success) {
            result.error = result.result.error;
        }

        std::cout << "✅ 任务完成: " << task.id << " (成功: " << (result.success ? "是" : "否") << ")" << std::endl;
    }
    catch (const std::exception& e) {
        result.success = false;
        result.error = "任务执行异常: " + std::string(e.what());
        std::cerr << "❌ 任务执行异常: " << result.error << std::endl;
    }

    return result;
}
