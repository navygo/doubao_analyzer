#include "GPUManager.hpp"
#include <iostream>
#include <opencv2/cudawarping.hpp>

// 尝试包含CUDA头文件
#include <cuda_runtime.h>

namespace gpu
{
    // 静态成员初始化
    GPUInfo GPUManager::gpu_info_;
    bool GPUManager::initialized_ = false;

    bool GPUManager::initialize()
    {
        if (initialized_)
        {
            return gpu_info_.available;
        }

        initialized_ = true;
        gpu_info_.available = false;

        try
        {
            // 检查OpenCV是否编译了CUDA支持
            int num_devices = cv::cuda::getCudaEnabledDeviceCount();

            if (num_devices > 0)
            {
                gpu_info_.available = true;
                gpu_info_.device_id = 0; // 使用第一个GPU设备

                // 使用try-catch处理CUDA相关代码，运行时检测CUDA是否可用
                try {
                // 使用CUDA API获取实际设备信息
                cudaDeviceProp deviceProp;
                cudaError_t error = cudaGetDeviceProperties(&deviceProp, gpu_info_.device_id);

                if (error == cudaSuccess)
                {
                    // 获取设备名称
                    gpu_info_.name = deviceProp.name;

                    // 计算计算能力 (major.minor * 10)
                    gpu_info_.compute_capability = deviceProp.major * 10 + deviceProp.minor;

                    // 获取内存信息 (转换为MB)
                    gpu_info_.total_memory = deviceProp.totalGlobalMem / (1024 * 1024);

                    // 获取可用内存
                    size_t free_mem = 0, total_mem = 0;
                    error = cudaMemGetInfo(&free_mem, &total_mem);
                    if (error == cudaSuccess)
                    {
                        gpu_info_.free_memory = free_mem / (1024 * 1024);
                    }
                    else
                    {
                        // 如果无法获取可用内存，使用总内存的一半作为估计值
                        gpu_info_.free_memory = gpu_info_.total_memory / 2;
                    }
                }
                else
                {
                    // 如果CUDA API失败，使用默认值
                    gpu_info_.name = "CUDA GPU";
                    gpu_info_.compute_capability = 60; // 默认计算能力
                    gpu_info_.total_memory = 4096;     // 默认4GB
                    gpu_info_.free_memory = 2048;      // 默认可用2GB
                }
                } catch (...) {
                    // 如果CUDA不可用，使用OpenCV提供的基本信息
                    gpu_info_.name = "CUDA GPU (OpenCV)";
                    gpu_info_.compute_capability = 75; // 默认计算能力
                    gpu_info_.total_memory = 4096;     // 默认4GB
                    gpu_info_.free_memory = 2048;      // 默认可用2GB
                }

                std::cout << "✅ 检测到GPU设备: " << gpu_info_.name
                          << " (计算能力: " << gpu_info_.compute_capability / 10 << "." << gpu_info_.compute_capability % 10
                          << ", 总内存: " << gpu_info_.total_memory << "MB"
                          << ", 可用内存: " << gpu_info_.free_memory << "MB)" << std::endl;

                // 尝试设置GPU设备
                cv::cuda::setDevice(gpu_info_.device_id);

                return true;
            }
            else
            {
                std::cout << "⚠️ 未检测到CUDA兼容的GPU设备，将使用CPU处理图像" << std::endl;
                return false;
            }
        }
        catch (const cv::Exception &e)
        {
            std::cerr << "❌ 初始化GPU时出错: " << e.what() << std::endl;
            std::cout << "⚠️ 将回退到CPU处理模式" << std::endl;
            return false;
        }
        catch (const std::exception &e)
        {
            std::cerr << "❌ 初始化GPU时发生异常: " << e.what() << std::endl;
            std::cout << "⚠️ 将回退到CPU处理模式" << std::endl;
            return false;
        }
    }

    const GPUInfo &GPUManager::get_gpu_info()
    {
        if (!initialized_)
        {
            initialize();
        }
        return gpu_info_;
    }

    bool GPUManager::is_gpu_available()
    {
        if (!initialized_)
        {
            initialize();
        }
        return gpu_info_.available;
    }

    bool GPUManager::use_gpu_for_opencv()
    {
        if (!is_gpu_available())
        {
            return false;
        }

        try
        {
            // 设置OpenCV使用GPU
            cv::cuda::setDevice(gpu_info_.device_id);
            return true;
        }
        catch (const cv::Exception &e)
        {
            std::cerr << "❌ 设置OpenCV使用GPU失败: " << e.what() << std::endl;
            return false;
        }
    }

    cv::Mat GPUManager::create_gpu_mat(int rows, int cols, int type)
    {
        if (is_gpu_available())
        {
            try
            {
                // 创建UMat，它会在GPU上分配内存
                cv::UMat u_mat(rows, cols, type);
                // 将UMat转换为Mat，OpenCV会自动处理GPU和CPU之间的数据传输
                cv::Mat result;
                u_mat.copyTo(result);
                return result;
            }
            catch (const cv::Exception &e)
            {
                std::cerr << "❌ 创建GPU Mat失败: " << e.what() << std::endl;
                return cv::Mat();
            }
        }
        return cv::Mat();
    }

    cv::Mat GPUManager::copy_to_gpu(const cv::Mat &cpu_mat)
    {
        if (is_gpu_available() && !cpu_mat.empty())
        {
            try
            {
                // 创建UMat并从CPU Mat复制数据
                cv::UMat u_gpu_mat;
                cpu_mat.copyTo(u_gpu_mat);
                // 将UMat转换为Mat，OpenCV会自动处理GPU和CPU之间的数据传输
                cv::Mat result;
                u_gpu_mat.copyTo(result);
                return result;
            }
            catch (const cv::Exception &e)
            {
                std::cerr << "❌ 复制数据到GPU失败: " << e.what() << std::endl;
                return cv::Mat();
            }
        }
        return cv::Mat();
    }

    cv::Mat GPUManager::copy_to_cpu(const cv::Mat &gpu_mat)
    {
        if (!gpu_mat.empty())
        {
            try
            {
                cv::Mat cpu_mat;
                // 如果输入是UMat（GPU数据），则复制到CPU Mat
                // 如果输入已经是CPU Mat，则直接复制
                gpu_mat.copyTo(cpu_mat);
                return cpu_mat;
            }
            catch (const cv::Exception &e)
            {
                std::cerr << "❌ 从GPU复制数据失败: " << e.what() << std::endl;
                return cv::Mat();
            }
        }
        return cv::Mat();
    }

    cv::Mat GPUManager::resize_image(const cv::Mat &image, int max_size)
    {
        if (image.empty())
        {
            return cv::Mat();
        }

        int height = image.rows;
        int width = image.cols;

        if (std::max(height, width) <= max_size)
        {
            return image.clone();
        }

        double scale = static_cast<double>(max_size) / std::max(height, width);
        int new_width = static_cast<int>(width * scale);
        int new_height = static_cast<int>(height * scale);

        cv::Mat resized;

        // 尝试使用GPU加速
        if (is_gpu_available())
        {
            try
            {
                // 使用UMat进行GPU加速处理
                cv::UMat u_image = image.getUMat(cv::ACCESS_READ);
                cv::UMat u_resized;

                // 确保UMat对象正确初始化
                if (u_image.empty())
                {
                    throw cv::Exception(cv::Error::StsNullPtr, "GPU image is empty",
                                        "resize_image", __FILE__, __LINE__);
                }

                cv::resize(u_image, u_resized, cv::Size(new_width, new_height));

                // 检查结果是否有效
                if (u_resized.empty())
                {
                    throw cv::Exception(cv::Error::StsInternal, "GPU resize operation failed",
                                        "resize_image", __FILE__, __LINE__);
                }

                // 使用更安全的转换方式
                try
                {
                    u_resized.copyTo(resized);
                }
                catch (const cv::Exception &e)
                {
                    std::cerr << "⚠️ GPU到CPU转换失败: " << e.what() << std::endl;
                    // 清理资源并回退到CPU处理
                    u_image.release();
                    u_resized.release();
                    throw e;
                }

                // 确保释放UMat资源
                u_image.release();
                u_resized.release();

                return resized;
            }
            catch (const cv::Exception &e)
            {
                std::cerr << "⚠️ GPU图像缩放失败，回退到CPU处理: " << e.what() << std::endl;
                // 继续执行CPU版本
            }
            catch (const std::exception &e)
            {
                std::cerr << "⚠️ GPU处理出现异常，回退到CPU处理: " << e.what() << std::endl;
                // 继续执行CPU版本
            }
        }

        // CPU版本
        try
        {
            cv::resize(image, resized, cv::Size(new_width, new_height));
            return resized;
        }
        catch (const cv::Exception &e)
        {
            std::cerr << "❌ CPU图像缩放失败: " << e.what() << std::endl;
            return cv::Mat();
        }
    }

    std::vector<unsigned char> GPUManager::encode_image_to_jpeg(const cv::Mat &image, int quality)
    {
        std::vector<unsigned char> buffer;
        std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, quality};

        // 检查输入图像
        if (image.empty())
        {
            std::cerr << "⚠️ 输入图像为空，无法编码" << std::endl;
            return buffer;
        }

        // 尝试使用GPU加速
        if (is_gpu_available())
        {
            cv::UMat u_image;
            cv::UMat u_rgb;

            try
            {
                // 使用UMat进行GPU加速处理
                image.copyTo(u_image);

                // 确保UMat对象正确初始化
                if (u_image.empty())
                {
                    throw cv::Exception(cv::Error::StsNullPtr, "GPU image is empty",
                                        "encode_image_to_jpeg", __FILE__, __LINE__);
                }

                // 颜色空间转换
                cv::cvtColor(u_image, u_rgb, cv::COLOR_BGR2RGB);

                // 检查转换结果
                if (u_rgb.empty())
                {
                    throw cv::Exception(cv::Error::StsInternal, "GPU color conversion failed",
                                        "encode_image_to_jpeg", __FILE__, __LINE__);
                }

                // 下载回CPU并编码为JPEG
                cv::Mat rgb_image;
                try
                {
                    u_rgb.copyTo(rgb_image);
                }
                catch (const cv::Exception &e)
                {
                    std::cerr << "⚠️ GPU到CPU转换失败: " << e.what() << std::endl;
                    // 清理资源并回退到CPU处理
                    u_image.release();
                    u_rgb.release();
                    throw e;
                }

                // 确保释放UMat资源
                u_image.release();
                u_rgb.release();

                if (!rgb_image.empty())
                {
                    cv::imencode(".jpg", rgb_image, buffer, params);
                }

                return buffer;
            }
            catch (const cv::Exception &e)
            {
                std::cerr << "⚠️ GPU图像编码失败，回退到CPU处理: " << e.what() << std::endl;
                // 确保资源释放
                u_image.release();
                u_rgb.release();
                // 继续执行CPU版本
            }
            catch (const std::exception &e)
            {
                std::cerr << "⚠️ GPU处理出现异常，回退到CPU处理: " << e.what() << std::endl;
                // 确保资源释放
                u_image.release();
                u_rgb.release();
                // 继续执行CPU版本
            }
        }

        // CPU版本
        try
        {
            cv::imencode(".jpg", image, buffer, params);
        }
        catch (const cv::Exception &e)
        {
            std::cerr << "❌ CPU图像编码失败: " << e.what() << std::endl;
        }

        return buffer;
    }
}
