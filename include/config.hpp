#pragma once

#include <string>
#include <vector>

namespace config
{
    // API配置结构体
    struct ApiConfig
    {
        std::string base_url;
        std::string model_name;
        std::string api_key;
        bool use_ollama;

        // 构造函数，设置默认值
        ApiConfig() : base_url(""),
                      model_name(""),
                      api_key(""),
                      use_ollama(true) {}
    };

    // API配置
    extern const std::string BASE_URL;
    extern const std::string MODEL_NAME;
    extern const std::string API_KEY;

    // 默认值
    extern const int DEFAULT_MAX_TOKENS;
    extern const int DEFAULT_VIDEO_FRAMES;
    extern const int DEFAULT_MAX_FILES;
    extern const double DEFAULT_TEMPERATURE;

    // 超时设置（秒）
    extern const int CONNECTION_TIMEOUT;
    extern const int IMAGE_ANALYSIS_TIMEOUT;
    extern const int VIDEO_ANALYSIS_TIMEOUT;

    // 文件扩展名
    extern const std::vector<std::string> IMAGE_EXTENSIONS;
    extern const std::vector<std::string> VIDEO_EXTENSIONS;

    // 数据库配置
    extern const std::string DB_HOST;
    extern const std::string DB_USER;
    extern const std::string DB_PASSWORD;
    extern const std::string DB_NAME;
    extern const unsigned int DB_PORT;
}
