#pragma once

#include <opencv2/opencv.hpp>
#include <string>

namespace gpu {
    // GPU设备信息
    struct GPUInfo {
        bool available = false;
        int device_id = -1;
        std::string name;
        int compute_capability = 0;
        size_t total_memory = 0;
        size_t free_memory = 0;
    };

    // GPU管理器类
    class GPUManager {
    private:
        static GPUInfo gpu_info_;
        static bool initialized_;

    public:
        // 初始化GPU管理器，检测GPU是否可用
        static bool initialize();

        // 获取GPU信息
        static const GPUInfo& get_gpu_info();

        // 检查GPU是否可用
        static bool is_gpu_available();

        // 设置OpenCV使用GPU
        static bool use_gpu_for_opencv();

        // 创建在GPU上的Mat（如果GPU可用）
        static cv::Mat create_gpu_mat(int rows, int cols, int type);

        // 将Mat从CPU复制到GPU（如果GPU可用）
        static cv::Mat copy_to_gpu(const cv::Mat& cpu_mat);

        // 将Mat从GPU复制回CPU（如果需要）
        static cv::Mat copy_to_cpu(const cv::Mat& gpu_mat);

        // 使用GPU进行图像缩放（如果GPU可用）
        static cv::Mat resize_image(const cv::Mat& image, int max_size);

        // 使用GPU进行图像编码（如果GPU可用）
        static std::vector<unsigned char> encode_image_to_jpeg(const cv::Mat& image, int quality = 85);
    };
}
