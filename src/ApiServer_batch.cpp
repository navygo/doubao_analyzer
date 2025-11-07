
// 处理批量分析请求
ApiResponse ApiServer::handle_batch_analysis(const std::vector<ApiRequest>& requests)
{
    ApiResponse response;
    nlohmann::json timing_info = nlohmann::json::object();
    double total_start_time = utils::get_current_time();

    std::cout << "🔄 [批量分析] 开始处理 " << requests.size() << " 个媒体文件" << std::endl;
    std::cout << "⏰ [时间戳] 请求接收时间: " << utils::get_formatted_timestamp() << std::endl;

    try {
        // 创建任务列表
        std::vector<AnalysisTask> tasks;
        tasks.reserve(requests.size());

        for (size_t i = 0; i < requests.size(); ++i) {
            const auto& req = requests[i];

            AnalysisTask task;
            task.id = "batch_" + std::to_string(i) + "_" + utils::get_current_timestamp();
            task.media_url = req.media_url;
            task.media_type = req.media_type;
            task.prompt = req.prompt.empty() ? (req.media_type == "video" ? get_video_prompt() : get_image_prompt()) : req.prompt;
            task.max_tokens = req.max_tokens > 0 ? req.max_tokens : config::DEFAULT_MAX_TOKENS;
            task.video_frames = req.video_frames > 0 ? req.video_frames : config::DEFAULT_VIDEO_FRAMES;
            task.save_to_db = req.save_to_db;

            tasks.push_back(task);
        }

        // 添加任务到队列并获取future列表
        auto futures = TaskManager::getInstance().addTasks(tasks);

        // 等待所有任务完成
        std::vector<TaskResult> results;
        results.reserve(futures.size());

        for (auto& future : futures) {
            results.push_back(future.get());
        }

        // 构建响应数据
        nlohmann::json results_array = nlohmann::json::array();
        int success_count = 0;

        for (const auto& result : results) {
            nlohmann::json result_obj;
            result_obj["task_id"] = result.task_id;
            result_obj["success"] = result.success;

            if (result.success) {
                result_obj["content"] = result.result.content;
                result_obj["tags"] = result.result.tags;
                result_obj["response_time"] = result.result.response_time;
                result_obj["usage"] = result.result.usage;
                success_count++;
            } else {
                result_obj["error"] = result.error;
            }

            results_array.push_back(result_obj);
        }

        // 设置响应
        response.success = true;
        response.message = "批量分析完成，成功: " + std::to_string(success_count) + "/" + std::to_string(requests.size());
        response.data["results"] = results_array;
        response.data["summary"] = {
            {"total", requests.size()},
            {"successful", success_count},
            {"failed", requests.size() - success_count}
        };

        double total_time = utils::get_current_time() - total_start_time;
        timing_info["total_seconds"] = total_time;
        timing_info["pending_tasks"] = TaskManager::getInstance().getPendingTaskCount();
        timing_info["active_threads"] = TaskManager::getInstance().getActiveThreadCount();

        std::cout << "✅ [批量分析] 处理完成，成功: " << success_count << "/" << requests.size() << std::endl;
        std::cout << "⏰ [时间戳] 请求处理完成时间: " << utils::get_formatted_timestamp() << std::endl;
        std::cout << "🎉 [完成] 批量分析请求处理完成，总耗时: " << total_time << " 秒" << std::endl;
    }
    catch (const std::exception& e) {
        response.success = false;
        response.message = "批量分析失败: " + std::string(e.what());
        response.error = "Batch analysis error";

        std::cerr << "❌ [批量分析] 异常: " << e.what() << std::endl;
    }

    response.data["timing"] = timing_info;
    response.response_time = utils::get_current_time() - total_start_time;

    return response;
}
