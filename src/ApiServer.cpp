#include "ApiServer.hpp"
#include "utils.hpp"
#include "ConfigManager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <chrono>

// HTTP服务器简单实现（基于socket）
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

// 从main.cpp中提取的提示词函数
std::string get_image_prompt()
{
    return R"(请仔细观察图片内容，为图片生成合适的标签。要求：
1. 仔细观察图片的各个细节
2. 生成的标签要准确反映图片内容
3. 标签数量不超过5个
4. 输出格式：通过分析图片，生成的标签为：['标签1', '标签2', '标签3'])";
}

std::string get_video_prompt()
{
    return R"(请仔细观察视频的关键帧内容，为视频生成合适的标签。要求：
1. 综合分析视频的整体内容和关键帧
2. 生成的标签要准确反映视频的主题、场景、动作等
3. 标签数量不超过8个
4. 输出格式：通过分析视频，生成的标签为：['标签1', '标签2', '标签3'])";
}

ApiServer::ApiServer(const std::string &api_key, int port, const std::string &host)
    : api_key_(api_key), port_(port), host_(host)
{
    // 初始化分析器
    analyzer_ = std::make_unique<DoubaoMediaAnalyzer>(api_key);
}

ApiServer::~ApiServer()
{
    stop();
}

bool ApiServer::initialize()
{
    // 测试API连接
    if (!analyzer_->test_connection())
    {
        std::cerr << "❌ API连接测试失败" << std::endl;
        return false;
    }

    // 初始化数据库
    if (!analyzer_->initialize_database())
    {
        std::cerr << "❌ 数据库初始化失败" << std::endl;
        return false;
    }

    std::cout << "✅ API服务器初始化成功" << std::endl;
    return true;
}

void ApiServer::start()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // 创建socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cerr << "❌ socket创建失败: " << strerror(errno) << std::endl;
        return;
    }

    // 设置socket选项
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        std::cerr << "❌ setsockopt失败: " << strerror(errno) << std::endl;
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(host_.c_str());
    address.sin_port = htons(port_);

    // 绑定socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "❌ 绑定失败: " << strerror(errno) << std::endl;
        return;
    }

    // 监听连接
    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "❌ 监听失败: " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "🚀 API服务器已启动，监听地址: " << host_ << ":" << port_ << std::endl;
    std::cout << "📋 可用的API路由:" << std::endl;
    std::cout << "   - POST /api/analyze : 分析图片或视频" << std::endl;
    std::cout << "   - POST /api/query : 查询已分析的结果" << std::endl;
    std::cout << "   - GET /api/status : 获取服务器状态" << std::endl;

    // 主循环，接受和处理连接
    while (true)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            std::cerr << "❌ 接受连接失败: " << strerror(errno) << std::endl;
            continue;
        }

        std::cout << "✅ 接受新连接" << std::endl;

        // 读取请求
        char buffer[4096] = {0};
        int valread = read(new_socket, buffer, 4096);
        if (valread <= 0)
        {
            close(new_socket);
            continue;
        }

        std::string request(buffer);
        std::cout << "📥 收到请求: " << request << std::endl;

        // 提取请求体
        std::string request_body;
        std::string request_path = "/"; // 默认路径

        // 提取请求路径
        size_t path_start = request.find(" ");
        if (path_start != std::string::npos)
        {
            size_t path_end = request.find(" ", path_start + 1);
            if (path_end != std::string::npos)
            {
                request_path = request.substr(path_start + 1, path_end - path_start - 1);
            }
        }
        size_t body_start = request.find("\r\n\r\n");
        if (body_start != std::string::npos)
        {
            request_body = request.substr(body_start + 4);
        }
        else
        {
            // 如果没有找到请求体分隔符，尝试查找Content-Length
            size_t content_length_pos = request.find("Content-Length:");
            if (content_length_pos != std::string::npos)
            {
                size_t colon_pos = request.find(":", content_length_pos);
                size_t length_start = request.find_first_not_of(" ", colon_pos + 1);
                size_t length_end = request.find("\r\n", length_start);
                if (length_end != std::string::npos)
                {
                    std::string length_str = request.substr(length_start, length_end - length_start);
                    int content_length = std::stoi(length_str);
                    size_t body_pos = request.find("\r\n\r\n", length_end);
                    if (body_pos != std::string::npos)
                    {
                        request_body = request.substr(body_pos + 4, content_length);
                    }
                }
            }
        }

        // 解析请求并处理
        ApiResponse response = process_request(request_body, request_path);

        // 构建完整响应JSON
        nlohmann::json response_json_obj;
        response_json_obj["success"] = response.success;
        response_json_obj["message"] = response.message;
        response_json_obj["data"] = response.data;
        response_json_obj["response_time"] = response.response_time;
        if (!response.error.empty())
        {
            response_json_obj["error"] = response.error;
        }

        // 发送响应
        std::string response_json = response_json_obj.dump();
        
        // 构建HTTP响应
        std::string http_response = "HTTP/1.1 200 OK\r\n";
        http_response += "Content-Type: application/json\r\n";
        http_response += "Content-Length: " + std::to_string(response_json.length()) + "\r\n";
        http_response += "\r\n";
        http_response += response_json;
        
        send(new_socket, http_response.c_str(), http_response.length(), 0);
        std::cout << "📤 发送响应: " << response_json << std::endl;

        close(new_socket);
    }
}

void ApiServer::stop()
{
    std::cout << "🛑 API服务器已停止" << std::endl;
}

ApiResponse ApiServer::process_request(const std::string &request_json, const std::string &path)
{
    ApiResponse response;

    try
    {
        // 处理状态查询请求
        if (path == "/api/status")
        {
            response.success = true;
            response.message = "服务器状态查询成功";
            response.data = get_status();
            response.response_time = 0.0;
            return response;
        }

        // 处理查询请求
        if (path == "/api/query")
        {
            // 解析JSON请求
            nlohmann::json request_data = nlohmann::json::parse(request_json);

            ApiQueryRequest query_request;
            query_request.query_type = request_data.value("query_type", "all");
            query_request.tag = request_data.value("tag", "");
            query_request.file_type = request_data.value("file_type", "");
            query_request.start_date = request_data.value("start_date", "");
            query_request.end_date = request_data.value("end_date", "");
            query_request.limit = request_data.value("limit", 10);
            query_request.condition = request_data.value("condition", "");
            query_request.media_url = request_data.value("media_url", "");

            // 处理查询请求
            double start_time = utils::get_current_time();
            response = handle_query_request(query_request);
            response.response_time = utils::get_current_time() - start_time;
            return response;
        }

        // 处理分析请求
        if (path == "/api/analyze")
        {
            // 解析JSON请求
            nlohmann::json request_data = nlohmann::json::parse(request_json);

            // 检查必要字段
            if (!request_data.contains("media_type") || !request_data.contains("media_url"))
            {
                response.success = false;
                response.message = "请求缺少必要字段: media_type 和 media_url";
                response.error = "Invalid request format";
                return response;
            }

            ApiRequest request;
            request.media_type = request_data["media_type"].get<std::string>();
            request.media_url = request_data["media_url"].get<std::string>();
            request.prompt = request_data.value("prompt", "");
            request.max_tokens = request_data.value("max_tokens", 1500);
            request.video_frames = request_data.value("video_frames", 5);
            request.save_to_db = request_data.value("save_to_db", true);

            // 验证媒体类型
            if (request.media_type != "image" && request.media_type != "video")
            {
                response.success = false;
                response.message = "无效的媒体类型，必须是 'image' 或 'video'";
                response.error = "Invalid media type";
                return response;
            }

            // 处理请求
            double start_time = utils::get_current_time();

            if (request.media_type == "image")
            {
                response = handle_image_analysis(request);
            }
            else
            {
                response = handle_video_analysis(request);
            }

            response.response_time = utils::get_current_time() - start_time;
            return response;
        }

        // 未知路径
        response.success = false;
        response.message = "未知的API路径: " + path;
        response.error = "Unknown API path";
        response.response_time = 0.0;
    }
    catch (const std::exception &e)
    {
        response.success = false;
        response.message = "处理请求时发生异常: " + std::string(e.what());
        response.error = "Request processing error";
        response.response_time = 0.0;
    }

    return response;
}

ApiResponse ApiServer::handle_image_analysis(const ApiRequest &request)
{
    ApiResponse response;

    try
    {
        // 下载图片到临时文件
        std::string temp_file = "/tmp/api_image_" + std::string(utils::get_current_timestamp()) + ".jpg";
        if (!utils::download_file(request.media_url, temp_file))
        {
            response.success = false;
            response.message = "图片下载失败: " + request.media_url;
            response.error = "Image download failed";
            return response;
        }

        // 使用默认提示词或自定义提示词
        std::string prompt = request.prompt.empty() ? get_image_prompt() : request.prompt;

        // 分析图片
        AnalysisResult result = analyzer_->analyze_single_image(
            temp_file,
            prompt,
            request.max_tokens);

        // 清理临时文件
        std::filesystem::remove(temp_file);

        if (result.success)
        {
            response.success = true;
            response.message = "图片分析成功";
            response.data = {
                {"content", result.content},
                {"tags", analyzer_->extract_tags(result.content)},
                {"response_time", result.response_time},
                {"usage", result.usage}};

            // 保存到数据库
            if (request.save_to_db)
            {
                if (save_to_database(result, request.media_url, "image"))
                {
                    response.data["saved_to_db"] = true;
                }
                else
                {
                    response.data["saved_to_db"] = false;
                    response.message += "，但结果未保存到数据库";
                }
            }
        }
        else
        {
            response.success = false;
            response.message = "图片分析失败: " + result.error;
            response.error = result.error;
        }
    }
    catch (const std::exception &e)
    {
        response.success = false;
        response.message = "图片分析异常: " + std::string(e.what());
        response.error = "Image analysis error";
    }

    return response;
}

ApiResponse ApiServer::handle_video_analysis(const ApiRequest &request)
{
    ApiResponse response;
    nlohmann::json timing_info = nlohmann::json::object();
    double total_start_time = utils::get_current_time();

    std::cout << "🎬 [视频分析] 开始处理视频分析请求: " << request.media_url << std::endl;
    std::cout << "⏰ [时间戳] 请求接收时间: " << utils::get_formatted_timestamp() << std::endl;

    try
    {
        // 使用高效视频分析方法，无需下载整个视频
        double extraction_start_time = utils::get_current_time();
        std::cout << "🎬 [视频分析] 开始高效分析视频，无需完整下载" << std::endl;
        std::cout << "⏰ [时间戳] 分析开始时间: " << utils::get_formatted_timestamp() << std::endl;

        // 使用默认提示词或自定义提示词
        std::string prompt = request.prompt.empty() ? get_video_prompt() : request.prompt;

        // 分析视频
        double analysis_start_time = utils::get_current_time();
        std::cout << "🔍 [视频分析] 开始分析视频..." << std::endl;
        std::cout << "⏰ [时间戳] 分析开始时间: " << utils::get_formatted_timestamp() << std::endl;
        std::cout << "📊 [参数] 提示词长度: " << prompt.length() << " 字符" << std::endl;
        std::cout << "📊 [参数] 最大令牌数: " << request.max_tokens << std::endl;
        std::cout << "📊 [参数] 提取帧数: " << request.video_frames << std::endl;

        AnalysisResult result = analyzer_->analyze_video_efficiently(
            request.media_url,
            prompt,
            request.max_tokens,
            "keyframes"); // 使用关键帧提取方法

        double analysis_time = utils::get_current_time() - analysis_start_time;
        timing_info["analysis_seconds"] = analysis_time;
        std::cout << "✅ [视频分析] 分析完成，耗时: " << analysis_time << " 秒" << std::endl;
        std::cout << "⏰ [时间戳] 分析完成时间: " << utils::get_formatted_timestamp() << std::endl;

        // 高效分析方法无需清理临时文件（已自动处理）

        if (result.success)
        {
            // 保存到数据库
            double db_start_time = utils::get_current_time();
            if (request.save_to_db)
            {
                std::cout << "💾 [数据库] 开始保存分析结果到数据库..." << std::endl;
                std::cout << "⏰ [时间戳] 数据库保存开始时间: " << utils::get_formatted_timestamp() << std::endl;

                if (save_to_database(result, request.media_url, "video"))
                {
                    double db_time = utils::get_current_time() - db_start_time;
                    timing_info["database_seconds"] = db_time;
                    std::cout << "✅ [数据库] 保存完成，耗时: " << db_time << " 秒" << std::endl;
                    std::cout << "⏰ [时间戳] 数据库保存完成时间: " << utils::get_formatted_timestamp() << std::endl;

                    response.data["saved_to_db"] = true;
                }
                else
                {
                    std::cout << "❌ [数据库] 保存失败" << std::endl;
                    response.data["saved_to_db"] = false;
                    response.message += "，但结果未保存到数据库";
                }
            }
            else
            {
                std::cout << "⏭️ [数据库] 跳过保存（save_to_db=false）" << std::endl;
            }

            response.success = true;
            response.message = "视频分析成功";
            response.data = {
                {"content", result.content},
                {"tags", analyzer_->extract_tags(result.content)},
                {"response_time", result.response_time},
                {"usage", result.usage},
                {"timing", timing_info}};

            double total_time = utils::get_current_time() - total_start_time;
            std::cout << "🎉 [完成] 视频分析请求处理完成，总耗时: " << total_time << " 秒" << std::endl;
            std::cout << "⏰ [时间戳] 请求处理完成时间: " << utils::get_formatted_timestamp() << std::endl;
        }
        else
        {
            std::cout << "❌ [错误] 视频分析失败: " << result.error << std::endl;
            response.success = false;
            response.message = "视频分析失败: " + result.error;
            response.error = result.error;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "❌ [异常] 视频分析异常: " << e.what() << std::endl;
        response.success = false;
        response.message = "视频分析异常: " + std::string(e.what());
        response.error = "Video analysis error";
    }

    return response;
}

bool ApiServer::save_to_database(const AnalysisResult &result, const std::string &media_url, const std::string &media_type)
{
    try
    {
        // 创建一个新的AnalysisResult，包含媒体信息
        AnalysisResult modified_result = result;
        modified_result.raw_response["path"] = media_url;
        modified_result.raw_response["type"] = media_type;

        // 保存到数据库
        return analyzer_->save_result_to_database(modified_result);
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ 保存到数据库失败: " << e.what() << std::endl;
        return false;
    }
}

nlohmann::json ApiServer::get_status()
{
    nlohmann::json status;
    status["server_status"] = "running";
    status["api_key_set"] = !api_key_.empty();
    status["port"] = port_;
    status["host"] = host_;

    // 获取数据库统计信息
    try
    {
        status["database_stats"] = analyzer_->get_database_statistics();
    }
    catch (const std::exception &e)
    {
        status["database_stats"] = nlohmann::json{{"error", e.what()}};
    }

    return status;
}

ApiResponse ApiServer::handle_query_request(const ApiQueryRequest &request)
{
    ApiResponse response;

    try
    {
        std::vector<MediaAnalysisRecord> results;

        // 根据查询类型执行不同的查询
        if (request.query_type == "all")
        {
            results = analyzer_->query_database_results(request.condition);
        }
        else if (request.query_type == "tag")
        {
            if (request.tag.empty())
            {
                response.success = false;
                response.message = "查询类型为'tag'时，必须提供'tag'参数";
                response.error = "Missing tag parameter";
                return response;
            }
            results = analyzer_->query_by_tag(request.tag);
        }
        else if (request.query_type == "type")
        {
            if (request.file_type.empty())
            {
                response.success = false;
                response.message = "查询类型为'type'时，必须提供'file_type'参数";
                response.error = "Missing file_type parameter";
                return response;
            }
            results = analyzer_->query_by_type(request.file_type);
        }
        else if (request.query_type == "date_range")
        {
            if (request.start_date.empty() || request.end_date.empty())
            {
                response.success = false;
                response.message = "查询类型为'date_range'时，必须提供'start_date'和'end_date'参数";
                response.error = "Missing date parameters";
                return response;
            }
            results = analyzer_->query_by_date_range(request.start_date, request.end_date);
        }
        else if (request.query_type == "recent")
        {
            results = analyzer_->get_recent_results(request.limit);
        }
        else if (request.query_type == "url")
        {
            if (request.media_url.empty())
            {
                response.success = false;
                response.message = "查询类型为'url'时，必须提供'media_url'参数";
                response.error = "Missing media_url parameter";
                return response;
            }
            results = analyzer_->query_by_url(request.media_url);
        }
        else
        {
            response.success = false;
            response.message = "不支持的查询类型: " + request.query_type;
            response.error = "Unsupported query type";
            return response;
        }

        // 将结果转换为JSON
        nlohmann::json results_json = nlohmann::json::array();
        for (const auto &record : results)
        {
            nlohmann::json record_json;
            record_json["id"] = record.id;
            record_json["file_path"] = record.file_path;
            record_json["file_name"] = record.file_name;
            record_json["file_type"] = record.file_type;
            record_json["analysis_result"] = record.analysis_result;
            record_json["tags"] = record.tags;
            record_json["response_time"] = record.response_time;
            record_json["created_at"] = record.created_at;
            results_json.push_back(record_json);
        }

        response.success = true;
        response.message = "查询成功，共找到 " + std::to_string(results.size()) + " 条记录";
        response.data["results"] = results_json;
        response.data["count"] = results.size();
    }
    catch (const std::exception &e)
    {
        response.success = false;
        response.message = "查询失败: " + std::string(e.what());
        response.error = "Query error";
    }

    return response;
}