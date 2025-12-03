
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <atomic>
#include <functional>

// 视频元数据结构
struct VideoMetadata
{
    int width = 0;
    int height = 0;
    double duration = 0.0;
    double fps = 0.0;
    std::string codec;
    std::string url;
    int total_frames = 0;  // 添加总帧数字段

    bool is_valid() const
    {
        return width > 0 && height > 0 && duration > 0;
    }
};

// 帧分析结果
struct FrameAnalysis
{
    double timestamp;
    double brightness;
    double contrast;
    double edge_density;
    int dominant_hue;

    nlohmann::json to_json() const
    {
        nlohmann::json j;
        j["timestamp"] = timestamp;
        j["brightness"] = brightness;
        j["contrast"] = contrast;
        j["edge_density"] = edge_density;
        j["dominant_hue"] = dominant_hue;
        return j;
    }
};

// 视频分析结果
struct VideoAnalysisResult
{
    bool success = false;
    VideoMetadata metadata;
    std::string method;
    std::vector<FrameAnalysis> frame_analyses;

    // 整体特征
    double avg_brightness = 0.0;
    double avg_contrast = 0.0;
    double avg_edge_density = 0.0;
    double avg_hue_distribution = 0.0;

    std::string error;

    nlohmann::json to_json() const
    {
        nlohmann::json j;
        j["success"] = success;
        j["metadata"] = {
            {"width", metadata.width},
            {"height", metadata.height},
            {"duration", metadata.duration},
            {"fps", metadata.fps},
            {"codec", metadata.codec},
            {"url", metadata.url}};
        j["method"] = method;

        nlohmann::json frames_json = nlohmann::json::array();
        for (const auto &frame : frame_analyses)
        {
            frames_json.push_back(frame.to_json());
        }
        j["frame_analyses"] = frames_json;

        j["overall"] = {
            {"avg_brightness", avg_brightness},
            {"avg_contrast", avg_contrast},
            {"avg_edge_density", avg_edge_density},
            {"avg_hue_distribution", avg_hue_distribution}};

        if (!error.empty())
        {
            j["error"] = error;
        }

        return j;
    }
};

// 视频分类结果
struct VideoClassification
{
    std::string video_type;   // landscape, portrait, square
    std::string content_type; // outdoor_bright, studio_bright, dark_scene, etc.
    std::string quality;      // high, medium, low
    double confidence = 0.0;

    nlohmann::json to_json() const
    {
        nlohmann::json j;
        j["video_type"] = video_type;
        j["content_type"] = content_type;
        j["quality"] = quality;
        j["confidence"] = confidence;
        return j;
    }
};

class VideoKeyframeAnalyzer
{
private:
    std::string temp_dir_;

    // 线程池相关成员
    std::vector<std::thread> worker_threads_;
    std::queue<std::function<void()>> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    std::atomic<bool> stop_threads_{false};

    // 内部方法
    std::string execute_command(const std::string &cmd);
    bool create_temp_directory();
    void cleanup_temp_directory();
    FrameAnalysis analyze_frame(const cv::Mat &frame, double timestamp);

    // 线程池初始化
    void initialize_thread_pool(int num_threads = std::thread::hardware_concurrency());

    // 线程工作函数
    void worker_thread();

    // 并发处理帧
    std::vector<std::string> process_frames_concurrently(
        const std::vector<std::string>& frame_paths,
        int max_concurrency = std::thread::hardware_concurrency());

    // 单个帧的处理函数
    std::string process_single_frame(const std::string& frame_path);

public:
    VideoKeyframeAnalyzer();
    ~VideoKeyframeAnalyzer();

    // 获取视频元数据，无需完整下载
    VideoMetadata get_video_metadata(const std::string &video_url);

    // 提取关键帧，返回base64编码的图像列表
    std::vector<std::string> extract_keyframes(const std::string &video_url,
                                               int max_frames = 5,
                                               const std::string &output_format = "jpg");

    // 提取采样帧，返回base64编码的图像列表
    std::vector<std::string> extract_sample_frames(const std::string &video_url,
                                                   int num_samples = 5);

    // 分析视频内容
    VideoAnalysisResult analyze_video_content(const std::string &video_url,
                                              const std::string &method = "keyframes",
                                              int num_frames = 5);

    // 对视频进行分类
    std::pair<VideoClassification, VideoAnalysisResult> classify_video(const std::string &video_url);
};
