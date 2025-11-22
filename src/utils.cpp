#include "utils.hpp"
#include "config.hpp"
#include "GPUManager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iomanip>
#include <curl/curl.h>

namespace utils
{

    // 字符串工具
    std::string to_lower(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    std::vector<std::string> split(const std::string &str, char delimiter)
    {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delimiter))
        {
            if (!token.empty())
            {
                tokens.push_back(token);
            }
        }

        return tokens;
    }

    std::string trim(const std::string &str)
    {
        return trim(str, " \t\n\r");
    }

    std::string trim(const std::string &str, const std::string &chars_to_trim)
    {
        size_t start = str.find_first_not_of(chars_to_trim);
        if (start == std::string::npos)
            return "";

        size_t end = str.find_last_not_of(chars_to_trim);
        return str.substr(start, end - start + 1);
    }

    bool starts_with(const std::string &str, const std::string &prefix)
    {
        return str.size() >= prefix.size() &&
               str.compare(0, prefix.size(), prefix) == 0;
    }

    bool ends_with(const std::string &str, const std::string &suffix)
    {
        return str.size() >= suffix.size() &&
               str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    // 文件工具
    bool file_exists(const std::string &path)
    {
        return std::filesystem::exists(path);
    }

    std::string get_file_extension(const std::string &path)
    {
        std::filesystem::path p(path);
        return p.extension().string();
    }

    bool is_image_file(const std::string &path)
    {
        std::string ext = to_lower(get_file_extension(path));
        return std::find(config::IMAGE_EXTENSIONS.begin(),
                         config::IMAGE_EXTENSIONS.end(), ext) != config::IMAGE_EXTENSIONS.end();
    }

    bool is_video_file(const std::string &path)
    {
        std::string ext = to_lower(get_file_extension(path));
        return std::find(config::VIDEO_EXTENSIONS.begin(),
                         config::VIDEO_EXTENSIONS.end(), ext) != config::VIDEO_EXTENSIONS.end();
    }

    std::vector<std::string> find_media_files(const std::string &folder,
                                              const std::string &file_type,
                                              int max_files)
    {
        std::vector<std::string> files;

        try
        {
            for (const auto &entry : std::filesystem::directory_iterator(folder))
            {
                if (files.size() >= max_files)
                    break;

                if (entry.is_regular_file())
                {
                    std::string path = entry.path().string();

                    if (file_type == "all")
                    {
                        if (is_image_file(path) || is_video_file(path))
                        {
                            files.push_back(path);
                        }
                    }
                    else if (file_type == "image" && is_image_file(path))
                    {
                        files.push_back(path);
                    }
                    else if (file_type == "video" && is_video_file(path))
                    {
                        files.push_back(path);
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << "Error accessing folder: " << e.what() << std::endl;
        }

        return files;
    }

    // Base64编码
    std::string base64_encode(const std::vector<unsigned char> &data)
    {
        static const std::string base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        std::string encoded;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        for (const auto &byte : data)
        {
            char_array_3[i++] = byte;
            if (i == 3)
            {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; i < 4; i++)
                {
                    encoded += base64_chars[char_array_4[i]];
                }
                i = 0;
            }
        }

        if (i > 0)
        {
            for (j = i; j < 3; j++)
            {
                char_array_3[j] = '\0';
            }

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; j < i + 1; j++)
            {
                encoded += base64_chars[char_array_4[j]];
            }

            while (i++ < 3)
            {
                encoded += '=';
            }
        }

        return encoded;
    }

    std::string base64_encode_file(const std::string &file_path)
    {
        double start_time = get_current_time();
        std::ifstream file(file_path, std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("Cannot open file: " + file_path);
        }

        // 优化：直接获取文件大小，避免读取整个文件到内存
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        double file_info_time = get_current_time();
        std::cout << "⏰ [性能] 文件信息获取完成，耗时: " << (file_info_time - start_time) << " 秒" << std::endl;
        std::cout << "⏰ [性能] 原始文件大小: " << file_size << " 字节" << std::endl;

        // 如果图片过大，先进行压缩
        if (file_size > 512 * 1024) // 降低阈值到512KB
        {
            std::cout << "⏰ [性能] 文件过大，开始压缩处理..." << std::endl;
            double compress_start = get_current_time();

            // 使用OpenCV读取并压缩图片
            cv::Mat img = cv::imread(file_path);
            if (!img.empty())
            {
                double load_time = get_current_time();
                std::cout << "⏰ [性能] 图片加载完成，耗时: " << (load_time - compress_start) << " 秒" << std::endl;
                std::cout << "⏰ [性能] 图片尺寸: " << img.cols << "x" << img.rows << std::endl;

                // 进一步减小图片尺寸，提高处理速度
                int max_size = 384; // 降低到384像素，提高处理速度

                // 直接调整到目标尺寸，减少中间步骤
                if (img.cols > max_size || img.rows > max_size)
                {
                    double resize_start = get_current_time();
                    double scale = max_size / (double)std::max(img.cols, img.rows);
                    cv::Mat resized;
                    // 使用INTER_LINEAR插值，比INTER_AREA更快
                    cv::resize(img, resized, cv::Size(), scale, scale, cv::INTER_LINEAR);
                    img = resized;
                    double resize_end = get_current_time();
                    std::cout << "⏰ [性能] 图片缩放完成，耗时: " << (resize_end - resize_start) << " 秒" << std::endl;
                }

                // 降低图片质量，减小文件大小
                double encode_start = get_current_time();
                std::vector<uchar> jpeg_data;
                std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 65, cv::IMWRITE_JPEG_OPTIMIZE, 1};
                cv::imencode(".jpg", img, jpeg_data, params);
                double encode_end = get_current_time();
                std::cout << "⏰ [性能] 图片编码完成，耗时: " << (encode_end - encode_start) << " 秒" << std::endl;
                std::cout << "⏰ [性能] 压缩后大小: " << jpeg_data.size() << " 字节" << std::endl;

                // 如果压缩后仍然较大，进一步降低质量
                if (jpeg_data.size() > 150 * 1024) // 降低阈值到150KB
                {
                    std::cout << "⏰ [性能] 文件仍然过大，进行二次压缩..." << std::endl;
                    double reencode_start = get_current_time();
                    params = {cv::IMWRITE_JPEG_QUALITY, 50, cv::IMWRITE_JPEG_OPTIMIZE, 1};
                    jpeg_data.clear();
                    cv::imencode(".jpg", img, jpeg_data, params);
                    double reencode_end = get_current_time();
                    std::cout << "⏰ [性能] 二次压缩完成，耗时: " << (reencode_end - reencode_start) << " 秒" << std::endl;
                    std::cout << "⏰ [性能] 二次压缩后大小: " << jpeg_data.size() << " 字节" << std::endl;
                }

                double b64_start = get_current_time();
                std::string result = base64_encode(jpeg_data);
                double b64_end = get_current_time();
                std::cout << "⏰ [性能] Base64编码完成，耗时: " << (b64_end - b64_start) << " 秒" << std::endl;

                double total_time = get_current_time() - start_time;
                std::cout << "⏰ [性能] 图片处理总耗时: " << total_time << " 秒" << std::endl;

                return result;
            }
        }

        // 小图片直接读取和编码
        std::cout << "⏰ [性能] 文件较小，直接处理..." << std::endl;
        double read_start = get_current_time();
        std::vector<unsigned char> buffer(file_size);
        file.read(reinterpret_cast<char*>(buffer.data()), file_size);
        double read_end = get_current_time();
        std::cout << "⏰ [性能] 文件读取完成，耗时: " << (read_end - read_start) << " 秒" << std::endl;

        double b64_start = get_current_time();
        std::string result = base64_encode(buffer);
        double b64_end = get_current_time();
        std::cout << "⏰ [性能] Base64编码完成，耗时: " << (b64_end - b64_start) << " 秒" << std::endl;

        double total_time = get_current_time() - start_time;
        std::cout << "⏰ [性能] 图片处理总耗时: " << total_time << " 秒" << std::endl;

        return result;
    }

    std::vector<unsigned char> base64_decode(const std::string &encoded_string)
    {
        static const std::string base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        auto is_base64 = [](unsigned char c)
        {
            return (isalnum(c) || (c == '+') || (c == '/'));
        };

        std::string trimmed_string;
        // 移除所有空白字符
        for (auto c : encoded_string)
        {
            if (!isspace(c))
            {
                trimmed_string += c;
            }
        }

        size_t in_len = trimmed_string.size();
        size_t i = 0;
        size_t in = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<unsigned char> ret;

        while (in_len-- && (trimmed_string[in] != '=') && is_base64(trimmed_string[in]))
        {
            char_array_4[i++] = trimmed_string[in];
            in++;
            if (i == 4)
            {
                for (i = 0; i < 4; i++)
                {
                    char_array_4[i] = base64_chars.find(char_array_4[i]);
                }

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; (i < 3); i++)
                {
                    ret.push_back(char_array_3[i]);
                }
                i = 0;
            }
        }

        if (i)
        {
            for (size_t j = i; j < 4; j++)
            {
                char_array_4[j] = 0;
            }

            for (size_t j = 0; j < 4; j++)
            {
                char_array_4[j] = base64_chars.find(char_array_4[j]);
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (size_t j = 0; (j < i - 1); j++)
            {
                ret.push_back(char_array_3[j]);
            }
        }

        return ret;
    }

    // 图像处理
    std::vector<unsigned char> encode_image_to_jpeg(const cv::Mat &image, int quality)
    {
        // 使用GPU加速（如果可用）
        return gpu::GPUManager::encode_image_to_jpeg(image, quality);
    }

    cv::Mat resize_image(const cv::Mat &image, int max_size)
    {
        // 使用GPU加速（如果可用）
        return gpu::GPUManager::resize_image(image, max_size);
    }

    std::string optimize_image_for_ollama(const std::string& base64_data, const std::string& image_url)
    {
        try 
        {
            // 解码base64数据
            std::vector<unsigned char> image_data = base64_decode(base64_data);

            // 从内存中加载图像
            cv::Mat image = cv::imdecode(image_data, cv::IMREAD_COLOR);
            if (image.empty()) 
            {
                std::cout << "⚠️ 无法解码图片数据，返回原始数据" << std::endl;
                return base64_data;
            }

            // 确保图片格式是Ollama支持的格式（JPEG或PNG）
            std::string output_format = ".jpg"; // 默认使用JPEG格式

            // 如果原始格式是PNG，可以考虑保留PNG格式（对于透明度重要的图像）
            if (image_url.find("data:image/png") == 0) 
            {
                output_format = ".png";
            }

            // 调整图像大小以减少数据量
            int max_dimension = 512; // 从1024降低到512，显著减少数据量

            // 如果图像太大，进行缩放
            if (image.cols > max_dimension || image.rows > max_dimension) 
            {
                double scale = std::min(
                    max_dimension / static_cast<double>(image.cols),
                    max_dimension / static_cast<double>(image.rows)
                );

                cv::Mat resized_image;
                cv::resize(image, resized_image, cv::Size(), scale, scale, cv::INTER_LINEAR);
                image = resized_image;
            }

            // 编码图像
            std::vector<unsigned char> optimized_data;
            std::vector<int> params;

            if (output_format == ".png") 
            {
                // PNG压缩参数
                params = {cv::IMWRITE_PNG_COMPRESSION, 6};
                cv::imencode(output_format, image, optimized_data, params);
            } 
            else 
            {
                // JPEG压缩参数 - 降低质量以减小文件大小
                params = {cv::IMWRITE_JPEG_QUALITY, 60, cv::IMWRITE_JPEG_OPTIMIZE, 1};
                cv::imencode(output_format, image, optimized_data, params);

                // 如果JPEG仍然太大，进一步降低质量
                if (optimized_data.size() > 150 * 1024) // 降低阈值到150KB
                {
                    params = {cv::IMWRITE_JPEG_QUALITY, 70, cv::IMWRITE_JPEG_OPTIMIZE, 1};
                    optimized_data.clear();
                    cv::imencode(output_format, image, optimized_data, params);
                }
            }

            // 重新编码为base64
            return base64_encode(optimized_data);
        } 
        catch (const std::exception& e) 
        {
            std::cout << "⚠️ 图片优化失败: " << e.what() << "，返回原始数据" << std::endl;
            return base64_data;
        }
    }

    // JSON工具
    nlohmann::json parse_json(const std::string &json_str)
    {
        return nlohmann::json::parse(json_str);
    }

    std::string json_to_string(const nlohmann::json &j)
    {
        return j.dump();
    }

    // 标签提取
    std::vector<std::string> extract_tags(const std::string &content)
    {
        std::vector<std::string> tags;

        try
        {
            // 查找数组格式 ['tag1', 'tag2']
            size_t start = content.find("['");
            size_t end = content.find("']");

            if (start != std::string::npos && end != std::string::npos && start < end)
            {
                std::string tags_str = content.substr(start + 2, end - start - 2);
                auto temp_tags = utils::split(tags_str, ',');

                for (const auto &tag : temp_tags)
                {
                    std::string clean_tag = utils::trim(tag);
                    clean_tag = trim(clean_tag, "'\"");
                    if (!clean_tag.empty())
                    {
                        tags.push_back(clean_tag);
                    }
                }

                if (!tags.empty())
                    return tags;
            }

            // 正则表达式匹配其他格式
            std::regex pattern1(R"(标签[：:]\s*([^。，！？!?]+))");
            std::regex pattern2(R"(['"]([^'"]+)['"])");
            std::regex pattern3(R"(([^,，、]+?)(?=,|，|、|$))");

            std::smatch matches;

            if (std::regex_search(content, matches, pattern1) && matches.size() > 1)
            {
                auto temp_tags = utils::split(matches[1].str(), ',');
                for (const auto &tag : temp_tags)
                {
                    std::string clean_tag = utils::trim(tag);
                    if (!clean_tag.empty())
                    {
                        tags.push_back(clean_tag);
                    }
                }
            }

            // 去重并限制数量
            std::sort(tags.begin(), tags.end());
            tags.erase(std::unique(tags.begin(), tags.end()), tags.end());

            if (tags.size() > 5)
            {
                tags.resize(5);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error extracting tags: " << e.what() << std::endl;
        }

        return tags;
    }

    // 时间工具
    double get_current_time()
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now.time_since_epoch()).count();
    }

    void sleep_seconds(int seconds)
    {
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
    }

    std::string replace_all(const std::string &str, const std::string &from, const std::string &to)
    {
        std::string result = str;
        size_t pos = 0;

        while ((pos = result.find(from, pos)) != std::string::npos)
        {
            result.replace(pos, from.length(), to);
            pos += to.length();
        }

        return result;
    }

    // 文件下载工具
    // 简单的URL缓存，避免重复下载
    static std::unordered_map<std::string, std::string> url_cache;
    static std::mutex cache_mutex;

    bool download_file(const std::string &url, const std::string &output_path)
    {
        // 检查缓存
        {
            std::lock_guard<std::mutex> lock(cache_mutex);
            auto it = url_cache.find(url);
            if (it != url_cache.end())
            {
                // 从缓存复制文件
                std::ifstream src(it->second, std::ios::binary);
                std::ofstream dst(output_path, std::ios::binary);
                if (src && dst)
                {
                    dst << src.rdbuf();
                    return true;
                }
            }
        }

        // 创建临时文件名用于缓存
        std::string temp_path = "/tmp/download_cache_" +
                                std::to_string(std::hash<std::string>{}(url)) + ".jpg";

        CURL *curl = curl_easy_init();
        if (!curl)
        {
            std::cerr << "❌ 初始化CURL失败" << std::endl;
            return false;
        }

        FILE *fp = fopen(temp_path.c_str(), "wb");
        if (!fp)
        {
            std::cerr << "❌ 无法打开输出文件: " << temp_path << std::endl;
            curl_easy_cleanup(curl);
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L); // 1分钟超时，比之前更快
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");

        // 启用TCP Fast Open
        curl_easy_setopt(curl, CURLOPT_TCP_FASTOPEN, 1L);

        CURLcode res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        fclose(fp);

        if (res != CURLE_OK)
        {
            std::cerr << "❌ 下载失败: " << curl_easy_strerror(res) << std::endl;
            std::filesystem::remove(temp_path); // 删除部分下载的文件
            return false;
        }

        // 复制到目标位置
        std::filesystem::copy_file(temp_path, output_path);

        // 添加到缓存
        {
            std::lock_guard<std::mutex> lock(cache_mutex);
            url_cache[url] = temp_path;
        }

        return true;
    }

    std::string get_current_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
                  1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

        return ss.str();
    }

    std::string get_formatted_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

        return ss.str();
    }

} // namespace utils
