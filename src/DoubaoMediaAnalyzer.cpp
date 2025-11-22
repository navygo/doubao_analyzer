#include "DoubaoMediaAnalyzer.hpp"
#include "utils.hpp"
#include "config.hpp"
#include "ConfigManager.hpp"
#include <curl/curl.h>
#include <sstream>
#include <iostream>

// HTTP回调函数
static size_t write_callback(void *contents, size_t size, size_t nmemb, std::string *response)
{
    size_t total_size = size * nmemb;
    response->append(static_cast<char *>(contents), total_size);
    return total_size;
}

// 判断是否使用Ollama API
bool DoubaoMediaAnalyzer::is_ollama_api(const std::string &url) const
{
    return (url.find("172.29.176.1:11434") != std::string::npos ||
            url.find("127.0.0.1:11434") != std::string::npos ||
            url.find("11434/api") != std::string::npos);
}

// 使用默认配置构造函数
DoubaoMediaAnalyzer::DoubaoMediaAnalyzer(const std::string &api_key)
    : api_key_(api_key), base_url_(config::BASE_URL), model_name_(config::MODEL_NAME)
{
    // 检查是否使用Ollama API
    use_ollama_ = is_ollama_api(base_url_);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    // 从配置文件加载数据库配置
    ConfigManager config_manager;
    if (config_manager.load_config())
    {
        const auto &db_config = config_manager.get_database_config();
        db_manager_ = std::make_unique<DatabaseManager>(db_config);
    }
    else
    {
        // 使用默认配置
        db_manager_ = std::make_unique<DatabaseManager>(
            config::DB_HOST,
            config::DB_USER,
            config::DB_PASSWORD,
            config::DB_NAME,
            config::DB_PORT);
    }

    // 初始化视频分析器
    try
    {
        video_analyzer_ = std::make_unique<VideoKeyframeAnalyzer>();
    }
    catch (const std::exception &e)
    {
        std::cerr << "初始化视频分析器失败: " << e.what() << std::endl;
    }
}

// 使用自定义API配置构造函数
DoubaoMediaAnalyzer::DoubaoMediaAnalyzer(const std::string &api_key, const std::string &base_url, const std::string &model_name)
    : api_key_(api_key), base_url_(base_url), model_name_(model_name)
{
    // 检查是否使用Ollama API
    use_ollama_ = is_ollama_api(base_url_);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    // 从配置文件加载数据库配置
    ConfigManager config_manager;
    if (config_manager.load_config())
    {
        const auto &db_config = config_manager.get_database_config();
        db_manager_ = std::make_unique<DatabaseManager>(db_config);
    }
    else
    {
        // 使用默认配置
        db_manager_ = std::make_unique<DatabaseManager>(
            config::DB_HOST,
            config::DB_USER,
            config::DB_PASSWORD,
            config::DB_NAME,
            config::DB_PORT);
    }

    // 初始化视频分析器
    try
    {
        video_analyzer_ = std::make_unique<VideoKeyframeAnalyzer>();
    }
    catch (const std::exception &e)
    {
        std::cerr << "初始化视频分析器失败: " << e.what() << std::endl;
    }
}

// 使用ApiConfig结构体构造函数
DoubaoMediaAnalyzer::DoubaoMediaAnalyzer(const config::ApiConfig &api_config)
    : api_key_(api_config.api_key), base_url_(api_config.base_url), model_name_(api_config.model_name), use_ollama_(api_config.use_ollama)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // 从配置文件加载数据库配置
    ConfigManager config_manager;
    if (config_manager.load_config())
    {
        const auto &db_config = config_manager.get_database_config();
        db_manager_ = std::make_unique<DatabaseManager>(db_config);
    }
    else
    {
        // 使用默认配置
        db_manager_ = std::make_unique<DatabaseManager>(
            config::DB_HOST,
            config::DB_USER,
            config::DB_PASSWORD,
            config::DB_NAME,
            config::DB_PORT);
    }

    // 初始化视频分析器
    try
    {
        video_analyzer_ = std::make_unique<VideoKeyframeAnalyzer>();
    }
    catch (const std::exception &e)
    {
        std::cerr << "初始化视频分析器失败: " << e.what() << std::endl;
    }
}

DoubaoMediaAnalyzer::~DoubaoMediaAnalyzer()
{
    curl_global_cleanup();
}

bool DoubaoMediaAnalyzer::test_connection()
{
    try
    {
        // 首先检查Ollama服务是否可访问
        if (use_ollama_)
        {
            std::cout << "🔍 [检查] 正在检查Ollama服务状态..." << std::endl;

            // 简单的HTTP GET请求检查服务是否可用
            // std::string check_url = base_url_.substr(0, base_url_.find_last_of("/"));
            //
            std::string check_url = base_url_.substr(0, base_url_.rfind("/api"));
            // 结果: "http://localhost:11434"
            std::cout << "🔍 [检查] 检查URL: " << check_url << std::endl;

            try
            {
                std::vector<std::string> headers = {"Content-Type: application/json"};
                std::string response = make_http_request(check_url, "GET", "", headers, 5, false);
                if (!response.empty())
                {
                    std::cout << "✅ [检查] Ollama服务响应正常" << std::endl;
                }
                else
                {
                    std::cout << "❌ [检查] Ollama服务无响应" << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cout << "❌ [检查] Ollama服务检查失败: " << e.what() << std::endl;
            }
        }

        nlohmann::json payload = {
            {"model", model_name_},
            {"messages", {{{"role", "user"}, {"content", "请回复'连接测试成功'"}}}},
            {"max_tokens", 50}};

        auto result = send_analysis_request(payload, config::CONNECTION_TIMEOUT);

        if (result.success)
        {
            if (use_ollama_)
            {
                std::cout << "✅ Ollama API连接正常" << std::endl;
            }
            else
            {
                std::cout << "✅ 豆包API连接正常" << std::endl;
            }
            return true;
        }
        else
        {
            std::string api_type = use_ollama_ ? "Ollama" : "豆包";
            std::cout << "❌ " << api_type << " API连接失败: " << result.error << std::endl;
            return false;
        }
    }
    catch (const std::exception &e)
    {
        std::string api_type = use_ollama_ ? "Ollama" : "豆包";
        std::cout << "❌ " << api_type << " 连接测试异常: " << e.what() << std::endl;
        return false;
    }
}

AnalysisResult DoubaoMediaAnalyzer::analyze_single_image(const std::string &image_path,
                                                         const std::string &prompt,
                                                         int max_tokens,
                                                         const std::string &model_name)
{
    AnalysisResult result;

    try
    {
        if (!utils::file_exists(image_path))
        {
            result.success = false;
            result.error = "图片文件不存在: " + image_path;
            return result;
        }

        // 记录图片编码开始时间
        double encode_start = utils::get_current_time();
        std::cout << "⏰ [性能] 开始编码图片: " << image_path << std::endl;
        std::string image_data = utils::base64_encode_file(image_path);
        double encode_end = utils::get_current_time();
        double encode_time = encode_end - encode_start;
        std::cout << "⏰ [性能] 图片编码完成，耗时: " << encode_time << " 秒" << std::endl;
        std::cout << "⏰ [性能] 编码后大小: " << image_data.size() << " 字节" << std::endl;

        // 按传递模型名称（如果有）或默认模型名称构建请求
        std::string original_model_name = model_name_;
        if (!model_name.empty())
        {
            original_model_name = model_name;
        }

        // 记录载荷构建开始时间
        double payload_start = utils::get_current_time();
        nlohmann::json payload = {
            {"model", original_model_name},
            {"messages", {{{"role", "user"}, {"content", {{{"type", "image_url"}, {"image_url", {{"url", "data:image/jpeg;base64," + image_data}}}}, {{"type", "text"}, {"text", prompt}}}}}}},
            {"max_tokens", max_tokens},
            {"temperature", config::DEFAULT_TEMPERATURE},
            {"stream", false}};
        double payload_end = utils::get_current_time();
        double payload_time = payload_end - payload_start;
        std::cout << "⏰ [性能] 载荷构建完成，耗时: " << payload_time << " 秒" << std::endl;

        // 记录API请求开始时间
        double request_start = utils::get_current_time();
        std::cout << "⏰ [性能] 开始发送API请求，使用模型: " << original_model_name << std::endl;
        result = send_analysis_request(payload, config::IMAGE_ANALYSIS_TIMEOUT);
        double request_end = utils::get_current_time();
        result.response_time = request_end - request_start;
        std::cout << "⏰ [性能] API请求完成，总耗时: " << result.response_time << " 秒" << std::endl;
    }
    catch (const std::exception &e)
    {
        result.success = false;
        result.error = "分析异常: " + std::string(e.what());
    }

    return result;
}

AnalysisResult DoubaoMediaAnalyzer::analyze_single_video(const std::string &video_path,
                                                         const std::string &prompt,
                                                         int max_tokens,
                                                         int num_frames,
                                                         const std::string &model_name)
{
    AnalysisResult result;

    try
    {
        if (!utils::file_exists(video_path))
        {
            result.success = false;
            result.error = "视频文件不存在: " + video_path;
            return result;
        }

        std::cout << "🎬 正在提取视频关键帧..." << std::endl;
        std::cout << "⏰ [时间戳] 帧提取开始时间: " << utils::get_formatted_timestamp() << std::endl;
        auto frames_start_time = utils::get_current_time();

        auto frames_base64 = extract_video_frames(video_path, num_frames);

        double frames_time = utils::get_current_time() - frames_start_time;
        std::cout << "⏱️ [耗时] 帧提取耗时: " << frames_time << " 秒" << std::endl;
        std::cout << "⏰ [时间戳] 帧提取完成时间: " << utils::get_formatted_timestamp() << std::endl;

        if (frames_base64.empty())
        {
            result.success = false;
            result.error = "无法从视频中提取有效帧";
            return result;
        }

        std::cout << "✅ 成功提取 " << frames_base64.size() << " 个关键帧" << std::endl;

        // 构建多图消息
        nlohmann::json content = nlohmann::json::array();
        content.push_back({{"type", "text"}, {"text", prompt}});

        for (size_t i = 0; i < frames_base64.size(); ++i)
        {
            content.push_back({{"type", "image_url"},
                               {"image_url", {{"url", "data:image/jpeg;base64," + frames_base64[i]}, {"detail", "low"}}}});

            content.push_back({{"type", "text"},
                               {"text", "这是视频的第" + std::to_string(i + 1) + "个关键帧"}});
        }
        // 按传递模型名称（如果有）或默认模型名称构建请求
        std::string original_model_name = model_name_;
        if (!model_name.empty())
        {
            original_model_name = model_name;
        }
        //
        nlohmann::json payload = {
            {"model", original_model_name},
            {"messages", {{{"role", "user"}, {"content", content}}}},
            {"max_tokens", max_tokens},
            {"temperature", config::DEFAULT_TEMPERATURE},
            {"stream", false}};

        std::cout << "📡 [API调用] 开始发送分析请求..." << std::endl;
        std::cout << "⏰ [时间戳] API请求开始时间: " << utils::get_formatted_timestamp() << std::endl;
        std::cout << "📊 [参数] 请求帧数: " << frames_base64.size() << std::endl;
        std::cout << "📊 [参数] 最大令牌数: " << max_tokens << std::endl;

        double start_time = utils::get_current_time();
        result = send_analysis_request(payload, config::VIDEO_ANALYSIS_TIMEOUT);
        result.response_time = utils::get_current_time() - start_time;

        std::cout << "⏱️ [耗时] API请求耗时: " << result.response_time << " 秒" << std::endl;
        std::cout << "⏰ [时间戳] API请求完成时间: " << utils::get_formatted_timestamp() << std::endl;

        if (result.success)
        {
            std::cout << "📊 [响应] 令牌使用情况: " << result.usage.dump() << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        result.success = false;
        result.error = "视频分析异常: " + std::string(e.what());
    }

    return result;
}

AnalysisResult DoubaoMediaAnalyzer::analyze_video_efficiently(const std::string &video_url,
                                                              const std::string &prompt,
                                                              int max_tokens,
                                                              const std::string &method,
                                                              int num_frames,
                                                              const std::string &model_name)
{
    AnalysisResult result;

    try
    {
        if (!video_analyzer_)
        {
            result.success = false;
            result.error = "视频分析器未初始化";
            return result;
        }

        std::cout << "🎬 正在高效分析视频（无需完整下载）..." << std::endl;
        std::cout << "⏰ [时间戳] 分析开始时间: " << utils::get_formatted_timestamp() << std::endl;
        std::cout << "🔗 视频URL: " << video_url << std::endl;
        std::cout << "📊 [方法] 使用 " << method << " 方法提取帧" << std::endl;

        auto frames_start_time = utils::get_current_time();

        // 提取关键帧或采样帧
        std::vector<std::string> frames_base64;
        if (method == "keyframes")
        {
            frames_base64 = video_analyzer_->extract_keyframes(video_url, num_frames); // 传递请求的帧数
        }
        else
        {
            frames_base64 = video_analyzer_->extract_sample_frames(video_url, num_frames); // 传递请求的帧数
        }

        double frames_time = utils::get_current_time() - frames_start_time;
        std::cout << "⏱️ [耗时] 帧提取耗时: " << frames_time << " 秒" << std::endl;
        std::cout << "⏰ [时间戳] 帧提取完成时间: " << utils::get_formatted_timestamp() << std::endl;

        if (frames_base64.empty())
        {
            result.success = false;
            result.error = "无法从视频中提取有效帧";
            return result;
        }

        std::cout << "✅ 成功提取 " << frames_base64.size() << " 个帧" << std::endl;

        // 获取视频元数据
        VideoMetadata metadata = video_analyzer_->get_video_metadata(video_url);
        std::cout << "📹 视频信息: " << metadata.width << "x" << metadata.height
                  << ", " << metadata.duration << "秒, " << metadata.fps << " FPS" << std::endl;

        // 构建多图消息
        nlohmann::json content = nlohmann::json::array();
        content.push_back({{"type", "text"}, {"text", prompt}});

        for (size_t i = 0; i < frames_base64.size(); ++i)
        {
            content.push_back({{"type", "image_url"},
                               {"image_url", {{"url", "data:image/jpeg;base64," + frames_base64[i]}, {"detail", "low"}}}});

            content.push_back({{"type", "text"},
                               {"text", "这是视频的第" + std::to_string(i + 1) + "个关键帧"}});
        }

        nlohmann::json payload = {
            {"model", model_name_},
            {"messages", {{{"role", "user"}, {"content", content}}}},
            {"max_tokens", max_tokens},
            {"temperature", config::DEFAULT_TEMPERATURE},
            {"stream", false}};

        std::cout << "📡 [API调用] 开始发送分析请求..." << std::endl;
        std::cout << "⏰ [时间戳] API请求开始时间: " << utils::get_formatted_timestamp() << std::endl;
        std::cout << "📊 [参数] 请求帧数: " << frames_base64.size() << std::endl;
        std::cout << "📊 [参数] 最大令牌数: " << max_tokens << std::endl;

        double start_time = utils::get_current_time();
        result = send_analysis_request(payload, config::VIDEO_ANALYSIS_TIMEOUT);
        result.response_time = utils::get_current_time() - start_time;

        std::cout << "⏱️ [耗时] API请求耗时: " << result.response_time << " 秒" << std::endl;
        std::cout << "⏰ [时间戳] API请求完成时间: " << utils::get_formatted_timestamp() << std::endl;

        if (result.success)
        {
            std::cout << "📊 [响应] 令牌使用情况: " << result.usage.dump() << std::endl;
        }

        // 将视频元数据添加到响应中
        result.raw_response["video_metadata"] = {
            {"width", metadata.width},
            {"height", metadata.height},
            {"duration", metadata.duration},
            {"fps", metadata.fps},
            {"codec", metadata.codec},
            {"url", metadata.url}};

        result.raw_response["extraction_method"] = method;
        result.raw_response["extraction_time"] = frames_time;
        result.raw_response["frames_extracted"] = frames_base64.size();
    }
    catch (const std::exception &e)
    {
        result.success = false;
        result.error = "高效视频分析异常: " + std::string(e.what());
    }

    return result;
}

std::vector<AnalysisResult> DoubaoMediaAnalyzer::batch_analyze(const std::string &media_folder,
                                                               const std::string &prompt,
                                                               int max_files,
                                                               const std::string &file_type)
{
    std::vector<AnalysisResult> results;

    auto media_files = utils::find_media_files(media_folder, file_type, max_files);

    if (media_files.empty())
    {
        std::cout << "❌ 在 " << media_folder << " 中未找到媒体文件" << std::endl;
        return results;
    }

    std::cout << "📁 找到 " << media_files.size() << " 个媒体文件进行批量分析" << std::endl;

    for (size_t i = 0; i < media_files.size(); ++i)
    {
        const auto &media_path = media_files[i];

        std::cout << " " << std::string(60, '=') << std::endl;
        std::cout
            << "📊 分析第 " << i + 1 << "/" << media_files.size()
            << " 个文件: " << std::filesystem::path(media_path).filename().string() << std::endl;

        try
        {
            auto file_size = std::filesystem::file_size(media_path);
            std::cout << "📏 文件大小: " << file_size << " 字节" << std::endl;
        }
        catch (...)
        {
            std::cout << "⚠️  无法读取文件大小信息" << std::endl;
        }

        AnalysisResult result;
        bool is_video = utils::is_video_file(media_path);

        if (is_video)
        {
            std::cout << "🎬 检测到视频文件" << std::endl;
            result = analyze_single_video(media_path, prompt);
        }
        else
        {
            std::cout << "🖼️  检测到图片文件" << std::endl;

            // 显示图片信息
            try
            {
                cv::Mat img = cv::imread(media_path);
                if (!img.empty())
                {
                    std::cout << "🖼️  图片尺寸: " << img.cols << "x" << img.rows << std::endl;
                }
                else
                {
                    std::cout << "⚠️  无法读取图片尺寸信息" << std::endl;
                }
            }
            catch (...)
            {
                std::cout << "⚠️  无法读取图片尺寸信息" << std::endl;
            }

            result = analyze_single_image(media_path, prompt);
        }

        if (result.success)
        {
            std::cout << "✅ 分析成功!" << std::endl;
            std::cout << "⏱️  响应时间: " << result.response_time << "秒" << std::endl;
            std::cout << "📝 分析结果: " << result.content << std::endl;

            auto tags = extract_tags(result.content);
            if (!tags.empty())
            {
                std::cout << "🏷️  提取标签: ";
                for (size_t j = 0; j < tags.size(); ++j)
                {
                    if (j > 0)
                        std::cout << ", ";
                    std::cout << tags[j];
                }
                std::cout << std::endl;
            }
        }
        else
        {
            std::cout << "❌ 分析失败: " << result.error << std::endl;
        }

        // 添加文件信息
        result.raw_response["file"] = std::filesystem::path(media_path).filename().string();
        result.raw_response["path"] = media_path;
        result.raw_response["type"] = is_video ? "video" : "image";

        results.push_back(result);

        // 添加延迟避免频繁调用
        if (i < media_files.size() - 1)
        {
            std::cout << "⏳ 等待2秒后继续..." << std::endl;
            utils::sleep_seconds(2);
        }
    }

    return results;
}

std::vector<std::string> DoubaoMediaAnalyzer::extract_tags(const std::string &content)
{
    return utils::extract_tags(content);
}

// 私有方法实现
std::vector<std::string> DoubaoMediaAnalyzer::extract_video_frames(const std::string &video_path, int num_frames)
{
    std::vector<std::string> frames_base64;

    try
    {
        cv::VideoCapture cap(video_path);
        if (!cap.isOpened())
        {
            throw std::runtime_error("无法打开视频文件");
        }

        int total_frames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
        double fps = cap.get(cv::CAP_PROP_FPS);
        double duration = (fps > 0) ? total_frames / fps : 0;

        std::cout << "📹 视频信息: " << total_frames << "帧, "
                  << fps << "FPS, " << duration << "秒" << std::endl;
        std::cout << "⏰ [时间戳] 视频信息获取完成: " << utils::get_formatted_timestamp() << std::endl;

        // 计算提取帧的位置
        std::vector<int> frame_positions;
        if (total_frames <= num_frames)
        {
            for (int i = 0; i < total_frames; ++i)
            {
                frame_positions.push_back(i);
            }
        }
        else
        {
            int step = total_frames / num_frames;
            for (int i = 0; i < num_frames; ++i)
            {
                frame_positions.push_back(i * step);
            }
            frame_positions.push_back(total_frames - 1); // 确保包含最后一帧
        }

        std::cout << "🔄 [帧处理] 开始提取关键帧..." << std::endl;
        std::cout << "⏰ [时间戳] 帧处理开始时间: " << utils::get_formatted_timestamp() << std::endl;

        for (size_t i = 0; i < frame_positions.size(); ++i)
        {
            double frame_start_time = utils::get_current_time();

            cap.set(cv::CAP_PROP_POS_FRAMES, frame_positions[i]);
            cv::Mat frame;
            bool ret = cap.read(frame);

            if (ret && !frame.empty())
            {
                // 调整帧大小以控制文件大小
                cv::Mat resized_frame = utils::resize_image(frame, 800);

                // 编码为base64
                auto jpeg_data = utils::encode_image_to_jpeg(resized_frame, 85);
                std::string frame_base64 = utils::base64_encode(jpeg_data);

                // 如果使用Ollama API，对帧数据进行优化
                if (use_ollama_)
                {
                    frame_base64 = utils::optimize_image_for_ollama(frame_base64, "data:image/jpeg;base64,");
                }

                frames_base64.push_back(frame_base64);

                double frame_time = utils::get_current_time() - frame_start_time;
                std::cout << "  ✅ 提取第" << i + 1 << "/" << frame_positions.size()
                          << "帧 (位置: " << frame_positions[i] << "/" << total_frames << "), 耗时: "
                          << frame_time << "秒" << std::endl;
            }
        }

        std::cout << "⏰ [时间戳] 帧处理完成时间: " << utils::get_formatted_timestamp() << std::endl;

        cap.release();
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("视频帧提取失败: " + std::string(e.what()));
    }

    return frames_base64;
}

AnalysisResult DoubaoMediaAnalyzer::send_analysis_request(const nlohmann::json &payload, int timeout)
{
    AnalysisResult result;

    try
    {
        std::vector<std::string> headers;

        // 根据API类型设置不同的请求头
        if (use_ollama_)
        {
            // Ollama API不需要Authorization头
            headers = {"Content-Type: application/json"};
        }
        else
        {
            // 豆包API需要Authorization头
            headers = {
                "Authorization: Bearer " + api_key_,
                "Content-Type: application/json"};
        }

        // 根据API类型调整payload格式
        nlohmann::json adjusted_payload;
        if (use_ollama_) // 先假设所有请求都不是Ollama API，便于调试
        {
            // 检查是否使用/api/generate端点
            bool is_generate_endpoint = (base_url_.find("/api/generate") != std::string::npos);

            if (is_generate_endpoint)
            {
                // Ollama /api/generate端点格式
                adjusted_payload["model"] = payload["model"];
                adjusted_payload["prompt"] = "请分析这张图片"; // 默认提示，将被实际提示覆盖
                adjusted_payload["stream"] = false;

                // 从messages中提取文本和图片
                if (payload.contains("messages") && !payload["messages"].empty())
                {
                    auto messages = payload["messages"][0];
                    if (messages.contains("content"))
                    {
                        auto content = messages["content"];
                        std::string prompt_text = "";

                        // 提取文本和图片
                        if (content.is_array())
                        {
                            for (const auto &item : content)
                            {
                                if (item.contains("type") && item["type"] == "text")
                                {
                                    prompt_text += item["text"].get<std::string>();
                                }
                            }
                        }
                        else if (content.is_string())
                        {
                            prompt_text = content.get<std::string>();
                        }

                        adjusted_payload["prompt"] = prompt_text;
                    }
                }

                // 添加选项
                if (payload.contains("max_tokens"))
                {
                    adjusted_payload["options"] = {
                        {"num_predict", payload["max_tokens"]}};
                }

                // 处理图片数据 - 优化图片处理
                if (payload.contains("messages") && !payload["messages"].empty())
                {
                    auto messages = payload["messages"][0];
                    if (messages.contains("content"))
                    {
                        auto content = messages["content"];
                        if (content.is_array())
                        {
                            std::vector<std::string> optimized_images;
                            for (const auto &item : content)
                            {
                                if (item.contains("type") && item["type"] == "image_url" && item.contains("image_url"))
                                {
                                    auto img_url = item["image_url"];
                                    if (img_url.contains("url"))
                                    {
                                        std::string url = img_url["url"].get<std::string>();
                                        if (url.find("data:image/") == 0 && url.find("base64,") != std::string::npos)
                                        {
                                            size_t pos = url.find("base64,") + 7;
                                            std::string base64_data = url.substr(pos);

                                            // 优化：对图片数据进行压缩和格式转换（如果需要）
                                            std::string optimized_data = utils::optimize_image_for_ollama(base64_data, url);
                                            optimized_images.push_back(optimized_data);
                                        }
                                    }
                                }
                            }

                            if (!optimized_images.empty())
                            {
                                adjusted_payload["images"] = optimized_images;
                            }
                        }
                    }
                }
                if (payload.contains("temperature"))
                {
                    if (!adjusted_payload.contains("options"))
                    {
                        adjusted_payload["options"] = nlohmann::json::object();
                    }
                    adjusted_payload["options"]["temperature"] = payload["temperature"];
                }
            }
            else
            {
                // Ollama /api/chat端点格式
                adjusted_payload["model"] = payload["model"];

                // 处理messages，确保content是字符串而不是数组
                nlohmann::json adjusted_messages = nlohmann::json::array();
                if (payload.contains("messages") && !payload["messages"].empty())
                {
                    for (const auto &msg : payload["messages"])
                    {
                        nlohmann::json adjusted_msg;
                        adjusted_msg["role"] = msg["role"];

                        // 将content数组转换为字符串
                        if (msg.contains("content"))
                        {
                            if (msg["content"].is_array())
                            {
                                std::string content_str = "";
                                std::vector<std::string> optimized_images;

                                for (const auto &item : msg["content"])
                                {
                                    if (item.contains("type") && item["type"] == "text" && item.contains("text"))
                                    {
                                        content_str += item["text"].get<std::string>();
                                    }
                                    else if (item.contains("type") && item["type"] == "image_url" && item.contains("image_url"))
                                    {
                                        // 提取图片URL并转换为base64
                                        auto img_url = item["image_url"];
                                        if (img_url.contains("url"))
                                        {
                                            std::string url = img_url["url"].get<std::string>();
                                            // 检查是否是base64格式的图片
                                            if (url.find("data:image/") == 0 && url.find("base64,") != std::string::npos)
                                            {
                                                // 提取base64数据部分
                                                size_t pos = url.find("base64,") + 7;
                                                std::string base64_data = url.substr(pos);

                                                // 优化：对图片数据进行压缩和格式转换
                                                std::string optimized_data = utils::optimize_image_for_ollama(base64_data, url);
                                                optimized_images.push_back(optimized_data);
                                            }
                                        }
                                    }
                                }

                                // 设置文本内容
                                adjusted_msg["content"] = content_str;

                                // 如果有图片，添加到消息中
                                if (!optimized_images.empty())
                                {
                                    adjusted_msg["images"] = optimized_images;
                                }
                            }
                            else
                            {
                                adjusted_msg["content"] = msg["content"];
                            }
                        }
                        adjusted_messages.push_back(adjusted_msg);
                    }
                }
                adjusted_payload["messages"] = adjusted_messages;
                adjusted_payload["stream"] = false;
                if (payload.contains("max_tokens"))
                {
                    adjusted_payload["options"] = {
                        {"num_predict", payload["max_tokens"]}};
                }
                if (payload.contains("temperature"))
                {
                    if (!adjusted_payload.contains("options"))
                    {
                        adjusted_payload["options"] = nlohmann::json::object();
                    }
                    adjusted_payload["options"]["temperature"] = payload["temperature"];
                }
            }
        }
        else
        {
            // 豆包API格式，直接使用原始payload
            adjusted_payload = payload;
        }

        std::string payload_str = adjusted_payload.dump();
        std::cout << "🔍 [调试] Ollama API请求URL: " << base_url_ << std::endl;
        std::cout << "⏰ [性能] 准备发送API请求，载荷大小: " << payload_str.size() << " 字节" << std::endl;

        bool enable_http2 = true; // 根据需要启用HTTP/2
        if (use_ollama_)          // 先假设所有请求都不是Ollama API，便于调试
        {
            enable_http2 = false; // Ollama API不支持HTTP/2
        }

        // 记录请求开始时间
        double request_start = utils::get_current_time();
        std::cout << "⏰ [性能] 开始发送HTTP请求，超时设置: " << timeout << " 秒" << std::endl;

        std::string response = make_http_request(base_url_, "POST", payload_str, headers, timeout, enable_http2);

        // 记录请求结束时间
        double request_end = utils::get_current_time();
        double request_time = request_end - request_start;
        std::cout << "⏰ [性能] HTTP请求完成，耗时: " << request_time << " 秒" << std::endl;
        std::cout << "⏰ [性能] 响应大小: " << response.size() << " 字节" << std::endl;

        // 记录响应处理开始时间
        double process_start = utils::get_current_time();
        auto result = process_response(response, 0); // response_time will be set by caller
        double process_end = utils::get_current_time();
        double process_time = process_end - process_start;
        std::cout << "⏰ [性能] 响应处理完成，耗时: " << process_time << " 秒" << std::endl;

        return result;
    }
    catch (const std::exception &e)
    {
        result.success = false;
        result.error = "HTTP请求异常: " + std::string(e.what());
        return result;
    }
}

AnalysisResult DoubaoMediaAnalyzer::process_response(const std::string &response_text, double response_time)
{
    AnalysisResult result;
    result.response_time = response_time;

    try
    {
        auto json_response = nlohmann::json::parse(response_text);

        // 根据API类型处理不同的响应格式
        if (use_ollama_)
        {
            // 检查是否使用/api/generate端点
            bool is_generate_endpoint = (base_url_.find("/api/generate") != std::string::npos);

            if (is_generate_endpoint)
            {
                // Ollama /api/generate端点响应格式
                if (json_response.contains("response"))
                {
                    result.success = true;
                    result.content = json_response["response"].get<std::string>();

                    // Ollama可能不返回usage信息，创建一个空的
                    result.usage = nlohmann::json::object();

                    result.raw_response = json_response;
                }
                else
                {
                    result.success = false;
                    result.error = "Ollama /api/generate API响应格式异常: " + response_text;
                }
            }
            else
            {
                // Ollama /api/chat端点响应格式
                if (json_response.contains("message") && json_response["message"].contains("content"))
                {
                    result.success = true;
                    result.content = json_response["message"]["content"].get<std::string>();

                    // Ollama可能不返回usage信息，创建一个空的
                    result.usage = nlohmann::json::object();

                    result.raw_response = json_response;
                }
                else
                {
                    result.success = false;
                    result.error = "Ollama /api/chat API响应格式异常: " + response_text;
                }
            }
        }
        else
        {
            // 豆包API响应格式
            if (json_response.contains("choices") && json_response["choices"].is_array() &&
                !json_response["choices"].empty())
            {
                auto choice = json_response["choices"][0];
                if (choice.contains("message") && choice["message"].contains("content"))
                {
                    result.success = true;
                    result.content = choice["message"]["content"].get<std::string>();

                    if (json_response.contains("usage"))
                    {
                        result.usage = json_response["usage"];
                    }

                    result.raw_response = json_response;
                }
                else
                {
                    result.success = false;
                    result.error = "响应格式异常: 缺少content字段";
                }
            }
            else
            {
                result.success = false;
                result.error = "响应格式异常: " + response_text;
            }
        }
    }
    catch (const nlohmann::json::parse_error &e)
    {
        result.success = false;
        result.error = "JSON解析失败: " + std::string(e.what()) + " - Response: " + response_text;
    }
    catch (const std::exception &e)
    {
        result.success = false;
        result.error = "处理响应异常: " + std::string(e.what());
    }

    return result;
}

// 静态CURL句柄，用于连接复用
static CURL *shared_curl = nullptr;
static std::mutex curl_mutex;

std::string DoubaoMediaAnalyzer::make_http_request(const std::string &url,
                                                   const std::string &method,
                                                   const std::string &data,
                                                   const std::vector<std::string> &headers,
                                                   int timeout,
                                                   bool enable_http2)
{

    std::lock_guard<std::mutex> lock(curl_mutex);

    // 初始化或重用CURL句柄
    if (!shared_curl)
    {
        shared_curl = curl_easy_init();
        // 设置全局选项
        curl_easy_setopt(shared_curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(shared_curl, CURLOPT_TCP_KEEPIDLE, 60L);
        curl_easy_setopt(shared_curl, CURLOPT_TCP_KEEPINTVL, 30L);
        curl_easy_setopt(shared_curl, CURLOPT_FORBID_REUSE, 0L);

        // 添加更多性能优化选项
        curl_easy_setopt(shared_curl, CURLOPT_NOSIGNAL, 1L);        // 避免信号中断
        curl_easy_setopt(shared_curl, CURLOPT_TCP_NODELAY, 1L);     // 禁用Nagle算法，减少延迟
        curl_easy_setopt(shared_curl, CURLOPT_BUFFERSIZE, 102400L); // 增大缓冲区大小到100KB
    }

    std::string response;

    // 设置请求特定选项
    curl_easy_setopt(shared_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(shared_curl, CURLOPT_CUSTOMREQUEST, method.c_str());
    curl_easy_setopt(shared_curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(shared_curl, CURLOPT_POSTFIELDSIZE, data.length());
    curl_easy_setopt(shared_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(shared_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(shared_curl, CURLOPT_TIMEOUT, timeout);

    // 启用HTTP/2和压缩
    // 根据参数决定是否启用HTTP/2
    // curl_easy_setopt(shared_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);
    if (enable_http2)
    {
        curl_easy_setopt(shared_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);
    }
    // else
    // {
    //     // 如果不支持HTTP/2，使用HTTP/1.1
    //     curl_easy_setopt(shared_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    // }
    // 压缩
    curl_easy_setopt(shared_curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");

    // 设置headers
    struct curl_slist *header_list = nullptr;
    for (const auto &header : headers)
    {
        header_list = curl_slist_append(header_list, header.c_str());
    }
    curl_easy_setopt(shared_curl, CURLOPT_HTTPHEADER, header_list);

    // 执行请求
    double perform_start = utils::get_current_time();
    CURLcode res = curl_easy_perform(shared_curl);
    double perform_end = utils::get_current_time();
    double perform_time = perform_end - perform_start;

    // 获取请求统计信息
    double total_time;
    double namelookup_time;
    double connect_time;
    double appconnect_time;
    double pretransfer_time;
    double starttransfer_time;

    curl_easy_getinfo(shared_curl, CURLINFO_TOTAL_TIME, &total_time);
    curl_easy_getinfo(shared_curl, CURLINFO_NAMELOOKUP_TIME, &namelookup_time);
    curl_easy_getinfo(shared_curl, CURLINFO_CONNECT_TIME, &connect_time);
    curl_easy_getinfo(shared_curl, CURLINFO_APPCONNECT_TIME, &appconnect_time);
    curl_easy_getinfo(shared_curl, CURLINFO_PRETRANSFER_TIME, &pretransfer_time);
    curl_easy_getinfo(shared_curl, CURLINFO_STARTTRANSFER_TIME, &starttransfer_time);

    std::cout << "⏰ [性能] CURL执行完成，耗时: " << perform_time << " 秒" << std::endl;
    std::cout << "⏰ [性能] DNS解析: " << namelookup_time << " 秒" << std::endl;
    std::cout << "⏰ [性能] 建立连接: " << connect_time << " 秒" << std::endl;
    std::cout << "⏰ [性能] SSL握手: " << appconnect_time << " 秒" << std::endl;
    std::cout << "⏰ [性能] 传输准备: " << pretransfer_time << " 秒" << std::endl;
    std::cout << "⏰ [性能] 首字节响应: " << starttransfer_time << " 秒" << std::endl;
    std::cout << "⏰ [性能] 总耗时: " << total_time << " 秒" << std::endl;

    curl_slist_free_all(header_list);

    if (res != CURLE_OK)
    {
        std::string error_msg = "HTTP请求失败: " + std::string(curl_easy_strerror(res));

        // 添加更多调试信息
        long response_code;
        curl_easy_getinfo(shared_curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code > 0)
        {
            error_msg += " (HTTP状态码: " + std::to_string(response_code) + ")";
        }

        throw std::runtime_error(error_msg);
    }

    return response;
}
