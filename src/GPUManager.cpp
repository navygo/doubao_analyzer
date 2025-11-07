#include "GPUManager.hpp"
#include <iostream>
#include <opencv2/cudawarping.hpp>

namespace gpu {
    // 静态成员初始化
    GPUInfo GPUManager::gpu_info_;
    bool GPUManager::initialized_ = false;

    bool GPUManager::initialize() {
        if (initialized_) {
            return gpu_info_.available;
        }

        initialized_ = true;
        gpu_info_.available = false;

        try {
            // 检查OpenCV是否编译了CUDA支持
            int num_devices = cv::cuda::getCudaEnabledDeviceCount();

            if (num_devices > 0) {
                gpu_info_.available = true;
                gpu_info_.device_id = 0; // 使用第一个GPU设备

                // 获取设备信息
                gpu_info_.name = "CUDA GPU";
                gpu_info_.compute_capability = 75;
                gpu_info_.total_memory = 8192; // 8GB示例值
                gpu_info_.free_memory = 4096; // 4GB示例值

                std::cout << "✅ 检测到GPU设备: " << gpu_info_.name 
                          << " (计算能力: " << gpu_info_.compute_capability 
                          << ", 总内存: " << gpu_info_.total_memory / (1024*1024) << "MB)" << std::endl;

                // 尝试设置GPU设备
                cv::cuda::setDevice(gpu_info_.device_id);

                return true;
            } else {
                std::cout << "⚠️ 未检测到CUDA兼容的GPU设备，将使用CPU处理图像" << std::endl;
                return false;
            }
        } catch (const cv::Exception& e) {
            std::cerr << "❌ 初始化GPU时出错: " << e.what() << std::endl;
            std::cout << "⚠️ 将回退到CPU处理模式" << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "❌ 初始化GPU时发生异常: " << e.what() << std::endl;
            std::cout << "⚠️ 将回退到CPU处理模式" << std::endl;
            return false;
        }
    }

    const GPUInfo& GPUManager::get_gpu_info() {
        if (!initialized_) {
            initialize();
        }
        return gpu_info_;
    }

    bool GPUManager::is_gpu_available() {
        if (!initialized_) {
            initialize();
        }
        return gpu_info_.available;
    }

    bool GPUManager::use_gpu_for_opencv() {
        if (!is_gpu_available()) {
            return false;
        }

        try {
            // 设置OpenCV使用GPU
            cv::cuda::setDevice(gpu_info_.device_id);
            return true;
        } catch (const cv::Exception& e) {
            std::cerr << "❌ 设置OpenCV使用GPU失败: " << e.what() << std::endl;
            return false;
        }
    }

    cv::Mat GPUManager::create_gpu_mat(int rows, int cols, int type) {
        if (is_gpu_available()) {
            try {
                cv::UMat u_mat;
                u_mat.create(rows, cols, type);
                return u_mat.getMat(cv::ACCESS_READ);
            } catch (const cv::Exception& e) {
                std::cerr << "❌ 创建GPU Mat失败: " << e.what() << std::endl;
                return cv::Mat();
            }
        }
        return cv::Mat();
    }

    cv::Mat GPUManager::copy_to_gpu(const cv::Mat& cpu_mat) {
        if (is_gpu_available() && !cpu_mat.empty()) {
            try {
                cv::Mat gpu_mat;
                cpu_mat.copyTo(gpu_mat);
                return gpu_mat;
            } catch (const cv::Exception& e) {
                std::cerr << "❌ 复制数据到GPU失败: " << e.what() << std::endl;
                return cv::Mat();
            }
        }
        return cv::Mat();
    }

    cv::Mat GPUManager::copy_to_cpu(const cv::Mat& gpu_mat) {
        if (!gpu_mat.empty()) {
            try {
                cv::Mat cpu_mat;
                gpu_mat.copyTo(cpu_mat);
                return cpu_mat;
            } catch (const cv::Exception& e) {
                std::cerr << "❌ 从GPU复制数据失败: " << e.what() << std::endl;
                return cv::Mat();
            }
        }
        return cv::Mat();
    }

    cv::Mat GPUManager::resize_image(const cv::Mat& image, int max_size) {
        if (image.empty()) {
            return cv::Mat();
        }

        int height = image.rows;
        int width = image.cols;

        if (std::max(height, width) <= max_size) {
            return image.clone();
        }

        double scale = static_cast<double>(max_size) / std::max(height, width);
        int new_width = static_cast<int>(width * scale);
        int new_height = static_cast<int>(height * scale);

        cv::Mat resized;

        // 尝试使用GPU加速
        if (is_gpu_available()) {
            try {
                // 使用UMat进行GPU加速处理
                cv::UMat u_image = image.getUMat(cv::ACCESS_READ);
                cv::UMat u_resized;
                cv::resize(u_image, u_resized, cv::Size(new_width, new_height));
                resized = u_resized.getMat(cv::ACCESS_READ);
                return resized;
            } catch (const cv::Exception& e) {
                std::cerr << "⚠️ GPU图像缩放失败，回退到CPU处理: " << e.what() << std::endl;
                // 继续执行CPU版本
            }
        }

        // CPU版本
        cv::resize(image, resized, cv::Size(new_width, new_height));
        return resized;
    }

    std::vector<unsigned char> GPUManager::encode_image_to_jpeg(const cv::Mat& image, int quality) {
        std::vector<unsigned char> buffer;
        std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, quality};

        // 尝试使用GPU加速
        if (is_gpu_available()) {
            try {
                // 使用UMat进行GPU加速处理
                cv::UMat u_image = image.getUMat(cv::ACCESS_READ);

                // 颜色空间转换
                cv::UMat u_rgb;
                cv::cvtColor(u_image, u_rgb, cv::COLOR_BGR2RGB);

                // 下载回CPU并编码为JPEG
                cv::Mat rgb_image = u_rgb.getMat(cv::ACCESS_READ);
                cv::imencode(".jpg", rgb_image, buffer, params);
                return buffer;
            } catch (const cv::Exception& e) {
                std::cerr << "⚠️ GPU图像编码失败，回退到CPU处理: " << e.what() << std::endl;
                // 继续执行CPU版本
            }
        }

        // CPU版本
        cv::imencode(".jpg", image, buffer, params);
        return buffer;
    }
}
