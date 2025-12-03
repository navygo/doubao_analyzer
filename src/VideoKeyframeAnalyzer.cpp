#include "VideoKeyframeAnalyzer.hpp"
#include "utils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <array>
#include <cstdlib>
#include <algorithm>

VideoKeyframeAnalyzer::VideoKeyframeAnalyzer()
{
    if (!create_temp_directory())
    {
        throw std::runtime_error("无法创建临时目录");
    }
    std::cout << "临时目录: " << temp_dir_ << std::endl;

    // 初始化线程池
    initialize_thread_pool();
}

VideoKeyframeAnalyzer::~VideoKeyframeAnalyzer()
{
    // 停止所有工作线程
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_threads_ = true;
    }
    queue_condition_.notify_all();

    // 等待所有线程完成
    for (auto &thread : worker_threads_)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    cleanup_temp_directory();
}

bool VideoKeyframeAnalyzer::create_temp_directory()
{
    // 创建唯一的临时目录
    std::string temp_template = "/tmp/doubao_video_XXXXXX";
    std::array<char, 256> temp_dir;
    strncpy(temp_dir.data(), temp_template.c_str(), temp_template.size());
    temp_dir[temp_template.size()] = '\0';

    char *result = mkdtemp(temp_dir.data());
    if (result == nullptr)
    {
        return false;
    }

    temp_dir_ = std::string(result);
    return true;
}

void VideoKeyframeAnalyzer::cleanup_temp_directory()
{
    if (!temp_dir_.empty() && std::filesystem::exists(temp_dir_))
    {
        try
        {
            std::filesystem::remove_all(temp_dir_);
        }
        catch (const std::exception &e)
        {
            std::cerr << "清理临时目录失败: " << e.what() << std::endl;
        }
    }
}

// 初始化线程池
void VideoKeyframeAnalyzer::initialize_thread_pool(int num_threads)
{
    for (int i = 0; i < num_threads; ++i)
    {
        worker_threads_.emplace_back(&VideoKeyframeAnalyzer::worker_thread, this);
    }
}

// 线程工作函数
void VideoKeyframeAnalyzer::worker_thread()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_condition_.wait(lock, [this]
                                  { return !task_queue_.empty() || stop_threads_; });

            if (stop_threads_ && task_queue_.empty())
            {
                return;
            }

            task = std::move(task_queue_.front());
            task_queue_.pop();
        }

        task();
    }
}

std::string VideoKeyframeAnalyzer::execute_command(const std::string &cmd)
{
    std::array<char, 128> buffer;
    std::string result;

    std::cout << "执行命令: " << cmd << std::endl;

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() 失败");
    }

    try
    {
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        {
            result += buffer.data();
        }
    }
    catch (...)
    {
        pclose(pipe);
        throw;
    }

    int exit_code = pclose(pipe);
    if (exit_code != 0)
    {
        throw std::runtime_error("命令执行失败，退出码: " + std::to_string(exit_code));
    }

    return result;
}

VideoMetadata VideoKeyframeAnalyzer::get_video_metadata(const std::string &video_url)
{
    VideoMetadata metadata;
    metadata.url = video_url;

    try
    {
        // 使用ffprobe获取视频元数据
        std::string cmd = "ffprobe -v error -select_streams v:0 -show_entries "
                          "stream=width,height,duration,avg_frame_rate,codec_name,nb_frames "
                          "-of json \"" +
                          video_url + "\"";

        std::string result = execute_command(cmd);

        // 解析JSON结果
        auto json_result = nlohmann::json::parse(result);

        if (json_result.contains("streams") && json_result["streams"].is_array() &&
            !json_result["streams"].empty())
        {

            auto stream = json_result["streams"][0];

            if (stream.contains("width"))
                metadata.width = stream["width"];
            if (stream.contains("height"))
                metadata.height = stream["height"];
            if (stream.contains("duration"))
            {
                if (stream["duration"].is_string())
                {
                    metadata.duration = std::stod(stream["duration"].get<std::string>());
                }
                else
                {
                    metadata.duration = stream["duration"];
                }
            }
            if (stream.contains("codec_name"))
                metadata.codec = stream["codec_name"];

            if (stream.contains("avg_frame_rate"))
            {
                std::string fps_str = stream["avg_frame_rate"];
                // 解析分数形式的帧率，如 "30/1"
                size_t slash_pos = fps_str.find('/');
                if (slash_pos != std::string::npos)
                {
                    int numerator = std::stoi(fps_str.substr(0, slash_pos));
                    int denominator = std::stoi(fps_str.substr(slash_pos + 1));
                    metadata.fps = denominator > 0 ? static_cast<double>(numerator) / denominator : 0.0;
                }
                else
                {
                    metadata.fps = std::stod(fps_str);
                }
            }
        }

        // 获取总帧数
        if (json_result.contains("streams") && json_result["streams"].is_array() &&
            !json_result["streams"].empty())
        {
            auto stream = json_result["streams"][0];
            if (stream.contains("nb_frames"))
            {
                if (stream["nb_frames"].is_string())
                {
                    metadata.total_frames = std::stoi(stream["nb_frames"].get<std::string>());
                }
                else
                {
                    metadata.total_frames = stream["nb_frames"];
                }
            }
        }

        // 如果无法直接获取总帧数，尝试通过时长和帧率计算
        if (metadata.total_frames <= 0 && metadata.duration > 0 && metadata.fps > 0)
        {
            metadata.total_frames = static_cast<int>(metadata.duration * metadata.fps);
        }

        // 输出视频元数据，包括总帧数
        std::cout << "视频元数据: " << metadata.width << "x" << metadata.height
                  << ", " << metadata.duration << "秒, " << metadata.fps
                  << " FPS, 编解码器: " << metadata.codec;
        if (metadata.total_frames > 0)
        {
            std::cout << ", 总帧数: " << metadata.total_frames;
        }
        std::cout << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "获取视频元数据失败: " << e.what() << std::endl;
    }

    return metadata;
}

// 并发处理帧
std::vector<std::string> VideoKeyframeAnalyzer::process_frames_concurrently(
    const std::vector<std::string> &frame_paths,
    int max_concurrency)
{

    std::vector<std::future<std::string>> futures;
    std::vector<std::string> results;

    // 提交所有帧处理任务
    for (const auto &frame_path : frame_paths)
    {
        // 使用lambda函数捕获this指针，以便调用成员函数
        std::function<std::string()> task = [this, frame_path]()
        {
            return process_single_frame(frame_path);
        };

        // 创建packaged_task并获取future
        auto packaged_task = std::make_shared<std::packaged_task<std::string()>>(task);
        futures.push_back(packaged_task->get_future());

        // 将任务添加到队列
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            task_queue_.emplace([packaged_task]()
                                { (*packaged_task)(); });
        }
        queue_condition_.notify_one();
    }

    // 等待所有任务完成并收集结果
    for (auto &future : futures)
    {
        try
        {
            results.push_back(future.get());
        }
        catch (const std::exception &e)
        {
            std::cerr << "处理帧时出错: " << e.what() << std::endl;
        }
    }

    return results;
}

// 处理单个帧
std::string VideoKeyframeAnalyzer::process_single_frame(const std::string &frame_path)
{
    if (!std::filesystem::exists(frame_path))
    {
        return "";
    }

    try
    {
        // 读取图像
        cv::Mat frame = cv::imread(frame_path);
        if (frame.empty())
        {
            return "";
        }

        // 调整图像大小
        cv::Mat resized_frame = utils::resize_image(frame, 800);

        // 编码为JPEG并转换为base64
        auto jpeg_data = utils::encode_image_to_jpeg(resized_frame, 85);
        std::string frame_base64 = utils::base64_encode(jpeg_data);

        return frame_base64;
    }
    catch (const std::exception &e)
    {
        std::cerr << "处理帧 " << frame_path << " 时出错: " << e.what() << std::endl;
        return "";
    }
}

std::vector<std::string> VideoKeyframeAnalyzer::extract_keyframes(const std::string &video_url,
                                                                  int max_frames,
                                                                  const std::string &output_format)
{
    std::vector<std::string> frames_base64;

    try
    {
        // 移除帧数限制，允许根据参数动态调整
        //
        // 创建临时输出文件路径
        std::string output_pattern = temp_dir_ + "/keyframe_%03d." + output_format;

        // 先获取视频编码格式
        std::string codec_check_cmd = "ffprobe -v error -select_streams v:0 -show_entries stream=codec_name -of csv \"" + video_url + "\"";
        std::string codec_result = execute_command(codec_check_cmd);
        std::string codec = "";

        // 解析编码格式
        if (codec_result.find("h264") != std::string::npos)
        {
            codec = "h264";
        }
        else if (codec_result.find("hevc") != std::string::npos)
        {
            codec = "hevc";
        }

        std::string cmd;

        // 根据编码格式选择不同的提取策略
        if (codec == "hevc")
        {
            // HEVC编码使用场景变化检测 + 固定间隔采样
            cmd = "ffmpeg -i \"" + video_url + "\" -vf \"select=gt(scene\\\\,0.3)+eq(n\\\\,2)\" "
                                               "-vsync vfr -frames:v " +
                  std::to_string(max_frames) +
                  " -q:v 2 -y \"" + output_pattern + "\"";
        }
        else
        {
            // H.264等其他编码使用关键帧检测 fmpeg命令需要添加scale参数，ffmpeg命令中添加 scale=384:384
            // 质量参数-q:v从3改为 2
            cmd = "ffmpeg -i \"" + video_url + "\" -vf \"select=eq(pict_type\\\\,I),scale=384:384\" "
                                               "-vsync vfr -frames:v " +
                  std::to_string(max_frames) +
                  " -q:v 2 -y \"" + output_pattern + "\"";
        }

        execute_command(cmd);

        // 收集所有提取的帧文件路径
        std::vector<std::string> frame_paths;
        for (int i = 1; i <= max_frames; ++i)
        {
            std::string frame_path = temp_dir_ + "/keyframe_" +
                                     (i < 10 ? "00" : (i < 100 ? "0" : "")) +
                                     std::to_string(i) + "." + output_format;
            if (std::filesystem::exists(frame_path))
            {
                frame_paths.push_back(frame_path);
            }
        }

        // 使用并发处理这些帧
        auto concurrent_start = std::chrono::high_resolution_clock::now();
        frames_base64 = process_frames_concurrently(frame_paths);
        auto concurrent_end = std::chrono::high_resolution_clock::now();
        auto concurrent_duration = std::chrono::duration_cast<std::chrono::milliseconds>(concurrent_end - concurrent_start).count();

        std::cout << "并发帧处理耗时: " << concurrent_duration / 1000.0 << " 秒" << std::endl;

        // 如果关键帧数量为0 ,避免出现无结果，使用采样方法补充到5帧
        max_frames = 3;
        //
        if (frames_base64.size() == 0)
        {
            std::cout << "关键帧数量不足(" << frames_base64.size() << ")，使用采样方法补充到" << max_frames << "帧" << std::endl;

            // 获取视频元数据
            VideoMetadata metadata = get_video_metadata(video_url);

            if (metadata.duration > 0)
            {
                // 计算需要补充的帧数
                int remaining_frames = max_frames - frames_base64.size();

                // 计算采样间隔
                double interval = metadata.duration / (remaining_frames + 1);

                // 为每个采样点创建临时文件路径
                std::vector<std::string> sample_paths;
                for (int i = 1; i <= remaining_frames; ++i)
                {
                    std::string sample_path = temp_dir_ + "/sample_" +
                                              (i < 10 ? "00" : (i < 100 ? "0" : "")) +
                                              std::to_string(i) + ".jpg";
                    sample_paths.push_back(sample_path);
                }

                // 构建FFmpeg命令
                std::string cmd = "ffmpeg -i \"" + video_url + "\"";

                // 添加采样时间点
                for (int i = 0; i < remaining_frames; ++i)
                {
                    double timestamp = (i + 1) * interval;
                    cmd += " -ss " + std::to_string(timestamp) + " -vframes 1 \"" + sample_paths[i] + "\"";
                }

                cmd += " -y";

                // 执行命令
                execute_command(cmd);

                // 使用并发处理这些采样帧
                std::vector<std::string> sample_frames = process_frames_concurrently(sample_paths);

                // 将处理好的采样帧添加到结果中
                for (const auto &frame : sample_frames)
                {
                    if (!frame.empty())
                    {
                        frames_base64.push_back(frame);
                    }
                }
            }
        }

        std::cout << "成功提取 " << frames_base64.size() << " 个关键帧" << std::endl;

        // 获取视频元数据和关键帧总数
        try
        {
            VideoMetadata metadata = get_video_metadata(video_url);
            if (metadata.total_frames > 0)
            {
                // // 获取视频中的关键帧总数
                // std::string cmd = "ffprobe -v error -skip_frame nokey -select_streams v:0 -show_entries "
                //                   "frame=key_frame -of csv\"" +
                //                   video_url + "\"";
                // std::string result = execute_command(cmd);

                // // 计算关键帧总数
                // int total_keyframes = 0;
                // std::istringstream iss(result);
                // std::string line;
                // while (std::getline(iss, line))
                // {
                //     if (line.find("I") != std::string::npos)
                //     {
                //         total_keyframes++;
                //     }
                // }

                // 计算比例
                double keyframe_ratio = (static_cast<double>(frames_base64.size()) / metadata.total_frames) * 100;
                // double extracted_ratio = total_keyframes > 0 ? (static_cast<double>(frames_base64.size()) / total_keyframes) * 100 : 0;

                // 输出统计信息
                std::cout << "📊 [统计] 视频总帧数: " << metadata.total_frames << std::endl;
                // std::cout << "📊 [统计] 关键帧总数: " << total_keyframes << std::endl;
                std::cout << "📊 [统计] 抽取关键帧数: " << frames_base64.size() << std::endl;
                std::cout << "📊 [统计] 抽取帧占总帧数比例: " << std::fixed << std::setprecision(2)
                          << keyframe_ratio << "%" << std::endl;
                // std::cout << "📊 [统计] 抽取帧占关键帧总数比例: " << std::fixed << std::setprecision(2)
                //           << extracted_ratio << "%" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "获取视频元数据失败: " << e.what() << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "提取关键帧失败: " << e.what() << std::endl;
    }

    return frames_base64;
}
// 增加默认值 num_samples = 5
std::vector<std::string> VideoKeyframeAnalyzer::extract_sample_frames(const std::string &video_url,
                                                                      int num_samples)
{
    std::vector<std::string> frames_base64;

    try
    {
        // 移除帧数限制，允许根据参数动态调整
        //
        // 获取视频时长
        VideoMetadata metadata = get_video_metadata(video_url);

        if (metadata.duration <= 0)
        {
            std::cerr << "无法获取视频时长，使用关键帧方法" << std::endl;
            return extract_keyframes(video_url, num_samples);
        }

        // 计算采样间隔
        double interval = metadata.duration / (num_samples + 1);

        // 为每个采样点创建临时文件路径
        std::vector<std::string> frame_paths;
        for (int i = 1; i <= num_samples; ++i)
        {
            std::string frame_path = temp_dir_ + "/sample_" +
                                     (i < 10 ? "00" : (i < 100 ? "0" : "")) +
                                     std::to_string(i) + ".jpg";
            frame_paths.push_back(frame_path);
        }

        // 构建FFmpeg命令
        std::string cmd = "ffmpeg -i \"" + video_url + "\"";

        // 添加采样时间点
        for (int i = 0; i < num_samples; ++i)
        {
            double timestamp = (i + 1) * interval;
            cmd += " -ss " + std::to_string(timestamp) + " -vframes 1 \"" + frame_paths[i] + "\"";
        }

        cmd += " -y";

        // 执行命令
        execute_command(cmd);

        // 使用并发处理这些采样帧
        auto concurrent_start = std::chrono::high_resolution_clock::now();
        frames_base64 = process_frames_concurrently(frame_paths);
        auto concurrent_end = std::chrono::high_resolution_clock::now();
        auto concurrent_duration = std::chrono::duration_cast<std::chrono::milliseconds>(concurrent_end - concurrent_start).count();

        std::cout << "并发采样帧处理耗时: " << concurrent_duration / 1000.0 << " 秒" << std::endl;

        std::cout << "成功提取 " << frames_base64.size() << " 个采样帧" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "采样帧提取失败: " << e.what() << std::endl;
    }

    return frames_base64;
}

FrameAnalysis VideoKeyframeAnalyzer::analyze_frame(const cv::Mat &frame, double timestamp)
{
    FrameAnalysis analysis;
    analysis.timestamp = timestamp;

    try
    {
        // 转换为灰度图像
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // 计算亮度
        analysis.brightness = cv::mean(gray)[0];

        // 计算对比度
        cv::Scalar mean, stddev;
        cv::meanStdDev(gray, mean, stddev);
        analysis.contrast = stddev[0];

        // 检测边缘
        cv::Mat edges;
        cv::Canny(gray, edges, 50, 150);
        analysis.edge_density = static_cast<double>(cv::countNonZero(edges)) / edges.total();

        // 颜色分析
        cv::Mat hsv;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        // 计算色调直方图
        cv::Mat hist;
        int histSize = 180; // 色调范围是0-179
        float range[] = {0, 180};
        const float *histRange = {range};
        cv::calcHist(&hsv, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);

        // 找出主导色调
        double maxVal = 0;
        cv::Point maxLoc;
        cv::minMaxLoc(hist, nullptr, &maxVal, nullptr, &maxLoc);
        analysis.dominant_hue = maxLoc.y;
    }
    catch (const std::exception &e)
    {
        std::cerr << "分析帧失败: " << e.what() << std::endl;
    }

    return analysis;
}

VideoAnalysisResult VideoKeyframeAnalyzer::analyze_video_content(const std::string &video_url,
                                                                 const std::string &method,
                                                                 int num_frames)
{
    VideoAnalysisResult result;
    result.method = method;

    try
    {
        // 获取视频元数据
        result.metadata = get_video_metadata(video_url);

        if (!result.metadata.is_valid())
        {
            result.error = "无法获取有效的视频元数据";
            return result;
        }

        // 提取帧
        std::vector<std::string> frames_base64;
        if (method == "keyframes")
        {
            frames_base64 = extract_keyframes(video_url, num_frames);
        }
        else
        {
            frames_base64 = extract_sample_frames(video_url, num_frames);
        }

        if (frames_base64.empty())
        {
            result.error = "无法提取视频帧";
            return result;
        }

        // 分析帧内容
        for (size_t i = 0; i < frames_base64.size(); ++i)
        {
            // 从base64解码图像
            std::vector<unsigned char> jpeg_data = utils::base64_decode(frames_base64[i]);
            cv::Mat frame = cv::imdecode(jpeg_data, cv::IMREAD_COLOR);

            if (!frame.empty())
            {
                // 估算时间戳
                double timestamp = 0.0;
                if (result.metadata.duration > 0)
                {
                    timestamp = (static_cast<double>(i) / frames_base64.size()) * result.metadata.duration;
                }

                // 分析帧
                FrameAnalysis frame_analysis = analyze_frame(frame, timestamp);
                result.frame_analyses.push_back(frame_analysis);
            }
        }

        // 计算整体特征
        if (!result.frame_analyses.empty())
        {
            double sum_brightness = 0.0, sum_contrast = 0.0, sum_edge_density = 0.0, sum_hue = 0.0;

            for (const auto &analysis : result.frame_analyses)
            {
                sum_brightness += analysis.brightness;
                sum_contrast += analysis.contrast;
                sum_edge_density += analysis.edge_density;
                sum_hue += analysis.dominant_hue;
            }

            size_t count = result.frame_analyses.size();

            result.avg_brightness = sum_brightness / count;
            result.avg_contrast = sum_contrast / count;
            result.avg_edge_density = sum_edge_density / count;
            result.avg_hue_distribution = sum_hue / count;
        }

        result.success = true;
    }
    catch (const std::exception &e)
    {
        result.error = "视频分析异常: " + std::string(e.what());
    }

    return result;
}

std::pair<VideoClassification, VideoAnalysisResult> VideoKeyframeAnalyzer::classify_video(const std::string &video_url)
{
    VideoAnalysisResult analysis = analyze_video_content(video_url, "keyframes", 5);
    VideoClassification classification;

    if (!analysis.success)
    {
        classification.confidence = 0.0;
        classification.video_type = "unknown";
        classification.content_type = "unknown";
        classification.quality = "unknown";
        return {classification, analysis};
    }

    // 基于分析结果进行简单分类
    const VideoMetadata &metadata = analysis.metadata;

    // 根据宽高比判断视频类型
    if (metadata.width > 0 && metadata.height > 0)
    {
        double aspect_ratio = static_cast<double>(metadata.width) / metadata.height;
        if (aspect_ratio > 1.5)
        {
            classification.video_type = "landscape";
        }
        else if (aspect_ratio < 0.8)
        {
            classification.video_type = "portrait";
        }
        else
        {
            classification.video_type = "square";
        }
    }

    // 根据亮度、对比度和边缘密度判断内容类型
    double brightness = analysis.avg_brightness;
    double contrast = analysis.avg_contrast;
    double edge_density = analysis.avg_edge_density;

    if (brightness > 180)
    {
        if (edge_density > 0.15)
        {
            classification.content_type = "outdoor_bright";
        }
        else
        {
            classification.content_type = "studio_bright";
        }
    }
    else if (brightness < 80)
    {
        if (edge_density > 0.1)
        {
            classification.content_type = "dark_scene";
        }
        else
        {
            classification.content_type = "night_scene";
        }
    }
    else
    {
        if (edge_density > 0.12)
        {
            classification.content_type = "action_scene";
        }
        else
        {
            classification.content_type = "dialogue_scene";
        }
    }

    // 根据分辨率判断质量
    if (metadata.width >= 1920 && metadata.height >= 1080)
    {
        classification.quality = "high";
        classification.confidence = 0.8;
    }
    else if (metadata.width >= 1280 && metadata.height >= 720)
    {
        classification.quality = "medium";
        classification.confidence = 0.7;
    }
    else
    {
        classification.quality = "low";
        classification.confidence = 0.6;
    }

    return {classification, analysis};
}
