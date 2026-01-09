#include "ApiServer.hpp"
#include "Jwt.hpp"
#include "utils.hpp"
#include "ConfigManager.hpp"
#include "RefreshTokenStore.hpp"
#include "ExcelProcessor.hpp"
#include <cstdlib>
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
// https://www.json.cn/jsonzip/ 压缩并转义 的在线工具

std::string get_image_promptbak()
{
    return R"(请仔细观察图片内容，为图片生成全面的标签分析。要求：
1. 仔细观察图片的各个细节，包括主体、背景、颜色、风格、情感等
2. 生成最多3组不同的标签体系，每组包含4级分类，从概括到具体
3. 每组标签体系应从不同角度分析图片，如：内容主题、视觉风格、情感表达等
4. 请严格按照以下四级标签体系对图片进行分类：
 一级标签：选择最概括的主类别
 二级标签：在一级标签下选择更具体的子类别
 三级标签：在二级标签下选择更详细的分类
 四级标签：在三级标签下选择最精准的描述性标签
5. 输出格式：
第一组标签分析：['一级标签', '二级标签', '三级标签', '四级标签']
第二组标签分析：['一级标签', '二级标签', '三级标签', '四级标签']
第三组标签分析：['一级标签', '二级标签', '三级标签', '四级标签'])";
}

std::string get_video_promptbak()
{
    return R"(请仔细观察视频的关键帧内容，为视频生成全面的标签分析。要求：
1. 综合分析视频的整体内容、关键帧、场景变化、动作序列等
2. 生成最多3组不同的标签体系，每组包含4级分类，从概括到具体
3. 每组标签体系应从不同角度分析视频，如：内容主题、视觉风格、叙事结构等
4. 请严格按照以下四级标签体系对视频进行分类：
 一级标签：选择最概括的主类别
 二级标签：在一级标签下选择更具体的子类别
 三级标签：在二级标签下选择更详细的分类
 四级标签：在三级标签下选择最精准的描述性标签
5. 输出格式：
第一组标签分析：['一级标签', '二级标签', '三级标签', '四级标签']
第二组标签分析：['一级标签', '二级标签', '三级标签', '四级标签']
第三组标签分析：['一级标签', '二级标签', '三级标签', '四级标签'])";
}

/*
(请仔细观察内容（图片/视频），为其生成合适的标签。要求：1.仔细观察内容的各个细节和关键帧2.生成的标签要准确反映内容的主题、场景、动作等3.请严格按照以下优先级顺序对内容进行分类（按顺序匹配，一旦匹配就不再继续）：-VM创意秀-五运六气-健康生活-全球视野-合香珠-皇室驼奶-广场舞-骨力满满-慢病管理-厨房魔法屋-免疫超人-TIENS33-VM操作引导-反诈宣传-活力日常4.分类规则：-按照上述顺序依次检查内容是否符合每个类别-一旦找到匹配的类别，立即确定为主类别，不再继续检查后续类别-在确定主类别后，生成相应的二级和三级标签5.输出格式：通过分析，生成的标签为：['主类别','具体分类','详细描述']6.注意事项：-严格按照优先级顺序进行分类-确保标签准确反映内容的具体情况-即使内容可能符合多个类别，也只选择最先匹配的类别-对于视频内容，要综合考虑所有关键帧的整体情况)
*/
std::string get_image_prompth()
{
    return R"(请仔细观察内容（图片/视频），为其生成合适的标签。要求：
1. 仔细观察内容的各个细节和关键帧
2. 生成的标签要准确反映内容的主题、场景、动作等
3. 请严格按照以下优先级顺序对内容进行分类（按顺序匹配，一旦匹配就不再继续）：
   - VM创意秀
   - 五运六气
   - 健康生活
   - 全球视野
   - 合香珠
   - 皇室驼奶
   - 广场舞
   - 骨力满满
   - 慢病管理
   - 厨房魔法屋
   - 免疫超人
   - TIENS 33
   - VM操作引导
   - 反诈宣传
   - 活力日常

4. 分类规则：
   - 按照上述顺序依次检查内容是否符合每个类别
   - 一旦找到匹配的类别，立即确定为主类别，不再继续检查后续类别
   - 在确定主类别后，生成相应的二级和三级标签

5. 输出格式：通过分析，生成的标签为：['主类别', '具体分类', '详细描述']
6. 注意事项：
   - 严格按照优先级顺序进行分类
   - 确保标签准确反映内容的具体情况
   - 即使内容可能符合多个类别，也只选择最先匹配的类别
   - 对于视频内容，要综合考虑所有关键帧的整体情况)";
}

std::string get_video_prompth()
{
    return R"(请仔细观察内容（图片/视频），为其生成合适的标签。要求：
1. 仔细观察内容的各个细节和关键帧
2. 生成的标签要准确反映内容的主题、场景、动作等
3. 请严格按照以下优先级顺序对内容进行分类（按顺序匹配，一旦匹配就不再继续）：
   - VM创意秀
   - 五运六气
   - 健康生活
   - 全球视野
   - 合香珠
   - 皇室驼奶
   - 广场舞
   - 骨力满满
   - 慢病管理
   - 厨房魔法屋
   - 免疫超人
   - TIENS 33
   - VM操作引导
   - 反诈宣传
   - 活力日常

4. 分类规则：
   - 按照上述顺序依次检查内容是否符合每个类别
   - 一旦找到匹配的类别，立即确定为主类别，不再继续检查后续类别
   - 在确定主类别后，生成相应的二级和三级标签

5. 输出格式：通过分析，生成的标签为：['主类别', '具体分类', '详细描述']
6. 注意事项：
   - 严格按照优先级顺序进行分类
   - 确保标签准确反映内容的具体情况
   - 即使内容可能符合多个类别，也只选择最先匹配的类别
   - 对于视频内容，要综合考虑所有关键帧的整体情况)";
}

/*
(请仔细观察图片内容，为图片生成合适的标签。要求：1.仔细观察图片的各个细节2.生成的标签要准确反映图片的主题、场景、动作等3.请严格按照以下优先级顺序对图片进行分类（按顺序匹配，一旦匹配就不再继续）：-VM创意秀-五运六气-健康生活-全球视野-合香珠-皇室驼奶-广场舞-骨力满满-慢病管理-厨房魔法屋-免疫超人-TIENS33-VM操作引导-反诈宣传-活力日常4.分类规则：-按照上述顺序依次检查图片内容是否符合每个类别-一旦找到匹配的类别，立即确定为主类别，不再继续检查后续类别-在确定主类别后，生成相应的二级和三级标签5.输出格式：通过分析，生成的标签为：['主类别','具体分类','详细描述']6.注意事项：-严格按照优先级顺序进行分类-确保标签准确反映图片的具体内容-即使内容可能符合多个类别，也只选择最先匹配的类别)
 */
std::string get_image_promptg()
{
    return R"(请仔细观察图片内容，为图片生成合适的标签。要求：
1. 仔细观察图片的各个细节
2. 生成的标签要准确反映图片的主题、场景、动作等
3. 请严格按照以下优先级顺序对图片进行分类（按顺序匹配，一旦匹配就不再继续）：
   - VM创意秀
   - 五运六气
   - 健康生活
   - 全球视野
   - 合香珠
   - 皇室驼奶
   - 广场舞
   - 骨力满满
   - 慢病管理
   - 厨房魔法屋
   - 免疫超人
   - TIENS 33
   - VM操作引导
   - 反诈宣传
   - 活力日常

4. 分类规则：
   - 按照上述顺序依次检查图片内容是否符合每个类别
   - 一旦找到匹配的类别，立即确定为主类别，不再继续检查后续类别
   - 在确定主类别后，生成相应的二级和三级标签

5. 输出格式：通过分析，生成的标签为：['主类别', '具体分类', '详细描述']
6. 注意事项：
   - 严格按照优先级顺序进行分类
   - 确保标签准确反映图片的具体内容
   - 即使内容可能符合多个类别，也只选择最先匹配的类别)";
}

std::string get_video_promptg()
{
    return R"(请仔细观察视频的关键帧内容，为视频生成合适的标签。要求：
1. 综合分析视频的整体内容和关键帧
2. 生成的标签要准确反映视频的主题、场景、动作等
3. 请严格按照以下优先级顺序对视频进行分类（按顺序匹配，一旦匹配就不再继续）：
   - VM创意秀
   - 五运六气
   - 健康生活
   - 全球视野
   - 合香珠
   - 皇室驼奶
   - 广场舞
   - 骨力满满
   - 慢病管理
   - 厨房魔法屋
   - 免疫超人
   - TIENS 33
   - VM操作引导
   - 反诈宣传
   - 活力日常

4. 分类规则：
   - 按照上述顺序依次检查视频内容是否符合每个类别
   - 一旦找到匹配的类别，立即确定为主类别，不再继续检查后续类别
   - 在确定主类别后，生成相应的二级和三级标签

5. 输出格式：通过分析，生成的标签为：['主类别', '具体分类', '详细描述']
6. 注意事项：
   - 严格按照优先级顺序进行分类
   - 确保标签准确反映视频的具体内容
   - 即使内容可能符合多个类别，也只选择最先匹配的类别)";
}

std::string get_image_prompta()
{
    return R"(请仔细观察图片内容，为图片生成合适的标签。要求：
1. 仔细观察图片的各个细节
2. 生成的标签要准确反映图片内容
3. 标签数量不超过5个
4. 输出格式：通过分析图片，生成的标签为：['标签1', '标签2', '标签3'])";
}

std::string get_video_prompta()
{
    return R"(请仔细观察视频的关键帧内容，为视频生成合适的标签。要求：
1. 综合分析视频的整体内容和关键帧
2. 生成的标签要准确反映视频的主题、场景、动作等
3. 标签数量不超过5个
4. 输出格式：通过分析视频，生成的标签为：['标签1', '标签2', '标签3'])";
}

std::string get_image_promptf()
{
    return R"(请仔细观察图片内容，为图片生成合适的标签。要求：
1. 仔细观察图片的各个细节
2. 生成的标签要准确反映图片的主题、场景、动作等
3. 请严格按照以下指定标签类别对图片进行分类：

VM创意秀
五运六气
健康生活
全球视野
合香珠
皇室驼奶
广场舞
骨力满满
慢病管理
厨房魔法屋
免疫超人
TIENS 33
VM操作引导
反诈宣传
活力日常

4. 输出格式：通过分析，生成的标签为：['主类别', '具体分类', '详细描述']
5. 注意事项：
   - 必须从上述15个指定类别中选择最匹配的一个作为主类别
   - 根据图片内容，在主类别基础上生成二级和三级标签
   - 确保标签准确反映图片的具体内容
   - 如果图片内容涉及多个类别，选择最主要的一个)";
}

std::string get_video_promptf()
{
    return R"(请仔细观察视频的关键帧内容，为视频生成合适的标签。要求：
1. 综合分析视频的整体内容和关键帧
2. 生成的标签要准确反映视频的主题、场景、动作等
3. 请严格按照以下指定标签类别对视频进行分类：

VM创意秀
五运六气
健康生活
全球视野
合香珠
皇室驼奶
广场舞
骨力满满
慢病管理
厨房魔法屋
免疫超人
TIENS 33
VM操作引导
反诈宣传
活力日常

4. 输出格式：通过分析，生成的标签为：['主类别', '具体分类', '详细描述']
5. 注意事项：
   - 必须从上述15个指定类别中选择最匹配的一个作为主类别
   - 根据视频内容，在主类别基础上生成二级和三级标签
   - 确保标签准确反映视频的具体内容
   - 如果视频内容涉及多个类别，选择最主要的一个)";
}

std::string get_image_promptd()
{
    return R"(请仔细观察图片内容，为图片生成合适的标签。要求：
1. 仔细观察图片的各个细节
2. 生成的标签要准确反映图片的主题、场景、动作等
3. 请严格按照以下三级标签体系对进行分类：
 一级标签：选择最概括的主类别。
 二级标签：在一级标签下选择更具体的子类别。
 三级标签：在二级标签下选择最精准的描述性标签
4. 输出格式：通过分析，生成的标签为：['一级标签', '二级标签', '三级标签'])";
}

std::string get_video_promptd()
{
    return R"(请仔细观察视频的关键帧内容，为视频生成合适的标签。要求：
1. 综合分析视频的整体内容和关键帧
2. 生成的标签要准确反映视频的主题、场景、动作等
3. 请严格按照以下多级标签体系对视频进行分类：
 一级标签：选择最概括的主类别。
 二级标签：在一级标签下选择更具体的子类别。
 三级标签：在二级标签下选择最精准的描述性标签
4. 输出格式：通过分析，生成的标签为：['一级标签', '二级标签', '三级标签'])";
}

std::string get_image_prompt()
{
    return R"(请仔细观察图片内容，为图片生成最合适的一组标签。要求：
1. 仔细观察图片的各个细节，识别图片最核心要表达的内容
2. 从所有可能的标签组合中，选择最能概括图片主题、场景、动作的那一组
3. 请严格按照以下三级标签体系进行分类，只输出最佳的一组标签：
  一级标签：选择最概括的主类别
  二级标签：在一级标签下选择最相关的子类别
  三级标签：在二级标签下选择最精准的描述性标签
4. 输出格式：你必须严格且只输出以下格式，不要任何其他解释文字：
['一级标签', '二级标签', '三级标签'])";
}

std::string get_video_prompt()
{
    return R"(请仔细观察视频的关键帧内容，为视频生成最合适的一组标签。要求：
1. 仔细观察图片的各个细节，识别图片最核心要表达的内容
2. 从所有可能的标签组合中，选择最能概括图片主题、场景、动作的那一组
3. 请严格按照以下三级标签体系进行分类，只输出最佳的一组标签：
  一级标签：选择最概括的主类别
  二级标签：在一级标签下选择最相关的子类别
  三级标签：在二级标签下选择最精准的描述性标签
4. 输出格式：你必须严格且只输出以下格式，不要任何其他解释文字：
['一级标签', '二级标签', '三级标签'])";
}

ApiServer::ApiServer(const std::string &api_key, int port, const std::string &host)
    : api_key_(api_key), port_(port), host_(host), server_running_(false), max_concurrent_requests_(30)
{
    // 初始化分析器
    analyzer_ = std::make_unique<DoubaoMediaAnalyzer>(api_key);

    // 初始化任务管理器（使用16个工作线程）调用大模型需要传递 api_key
    TaskManager::getInstance().initialize(16, api_key);

    // 初始化并发请求处理
    server_running_ = true;

    // 创建请求处理工作线程
    size_t num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0)
        num_threads = 8; // 默认使用8个线程

    std::cout << "🚀 初始化API服务器并发处理，使用 " << num_threads << " 个工作线程" << std::endl;

    for (size_t i = 0; i < num_threads; ++i)
    {
        worker_threads_.emplace_back(&ApiServer::request_worker_thread, this);
    }
}

ApiServer::~ApiServer()
{
    stop();

    // 停止并发请求处理
    server_running_ = false;
    queue_condition_.notify_all();

    // 等待所有工作线程结束
    for (auto &thread : worker_threads_)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    std::cout << "🛑 所有API服务器工作线程已停止" << std::endl;
}

bool ApiServer::initialize()
{
    // 支持在测试/本地环境跳过外部 API 和数据库初始化检查
    const char *skip_env = std::getenv("SKIP_API_INIT");
    if (skip_env && std::string(skip_env) == "1")
    {
        std::cout << "⚠️ SKIP_API_INIT=1，跳过外部 API 与数据库初始化检查" << std::endl;
        return true;
    }

    // 测试API连接
    // if (!analyzer_->test_connection())
    // {
    //     std::cerr << "❌ API连接测试失败" << std::endl;
    //     return false;
    // }

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
    int server_fd;
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
    if (listen(server_fd, 128) < 0) // 增加监听队列大小
    {
        std::cerr << "❌ 监听失败: " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "🚀 API服务器已启动，监听地址: " << host_ << ":" << port_ << std::endl;
    std::cout << "📋 可用的API路由:" << std::endl;
    std::cout << "   - POST /api/auth : 获取JWT令牌" << std::endl;
    std::cout << "   - POST /api/auth/refresh : 刷新 access token，使用 refresh token 获取新的 access token" << std::endl;

    std::cout << "   - POST /api/analyze : 分析图片、视频、文本、文件或音频" << std::endl;
    std::cout << "   - POST /api/batch_analyze : 批量分析图片或视频" << std::endl;
    std::cout << "   - POST /api/excel_analyze : 分析Excel文件中的媒体URL" << std::endl;

    std::cout << "   - POST /api/query : 查询已分析的结果" << std::endl;
    std::cout << "   - GET /api/status : 获取服务器状态" << std::endl;
    std::cout << "🔄 服务器已启用并发处理，最大并发连接数: " << max_concurrent_requests_ << std::endl;

    // 主循环，接受连接
    while (true)
    {
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            std::cerr << "❌ 接受连接失败: " << strerror(errno) << std::endl;
            continue;
        }

        std::cout << "✅ 接受新连接，socket: " << new_socket << std::endl;

        // 检查当前并发请求数是否超过限制
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (request_queue_.size() >= max_concurrent_requests_)
            {
                std::cerr << "⚠️ 服务器繁忙，并发请求数已达上限: " << max_concurrent_requests_ << std::endl;

                // 发送服务器繁忙响应
                std::string busy_response = "HTTP/1.1 503 Service Unavailable\r\n";
                busy_response += "Content-Type: application/json\r\n";
                busy_response += "Content-Length: 85\r\n";
                busy_response += "\r\n";
                busy_response += "{\"success\":false,\"message\":\"服务器繁忙，请稍后再试\",\"error\":\"Service Unavailable\"}";

                send(new_socket, busy_response.c_str(), busy_response.length(), 0);
                close(new_socket);
                continue;
            }
        }

        // 将连接处理任务添加到队列
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            request_queue_.push([this, new_socket]()
                                { handle_connection(new_socket); });
        }

        // 通知工作线程有新请求
        queue_condition_.notify_one();

        // 请求处理已移至handle_connection函数，由工作线程并发处理
    }
}

void ApiServer::stop()
{
    std::cout << "🛑 API服务器已停止" << std::endl;
}

void ApiServer::request_worker_thread()
{
    while (server_running_)
    {
        std::function<void()> request_handler;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_condition_.wait(lock, [this]
                                  { return !server_running_ || !request_queue_.empty(); });

            if (!server_running_)
                break;

            if (request_queue_.empty())
                continue;

            request_handler = std::move(request_queue_.front());
            request_queue_.pop();
        }

        // 执行请求处理
        try
        {
            request_handler();
        }
        catch (const std::exception &e)
        {
            std::cerr << "❌ 请求处理异常: " << e.what() << std::endl;
        }
    }
}

void ApiServer::handle_connection(int client_socket)
{
    try
    {
        // 读取请求
        char buffer[4096] = {0};
        int valread = read(client_socket, buffer, 4096);
        if (valread <= 0)
        {
            close(client_socket);
            return;
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
        size_t body_start = request.find("\r\n\r\n"); // 而不是被截断的版本
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

        // 解析请求头并提取 Authorization（如果有）
        std::string auth_header;
        size_t headers_end = request.find("\r\n\r\n");
        if (headers_end != std::string::npos)
        {
            std::string headers = request.substr(0, headers_end);
            size_t auth_pos = headers.find("Authorization:");
            if (auth_pos != std::string::npos)
            {
                // 修复Authorization头查找
                size_t line_end = headers.find("\r\n", auth_pos);
                if (line_end == std::string::npos)
                    line_end = headers.length();
                std::string line = headers.substr(auth_pos, line_end - auth_pos);
                size_t colon = line.find(":");
                if (colon != std::string::npos)
                {
                    auth_header = line.substr(colon + 1);
                    // trim
                    while (!auth_header.empty() && (auth_header.front() == ' ' || auth_header.front() == '	'))
                        auth_header.erase(0, 1);
                    while (!auth_header.empty() && (auth_header.back() == ' ' || auth_header.back() == ' ' || auth_header.back() == ' '))
                        auth_header.pop_back();
                }
            }
        }

        // 解析请求并处理
        ApiResponse response = process_request(request_body, request_path, auth_header);

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

        // 构建HTTP响应（若未经授权则返回401）
        std::string http_response;
        if (response.error == "Unauthorized")
            http_response = "HTTP/1.1 401 Unauthorized\r\n"; // 确保有完整的\r\n
        else
            http_response = "HTTP/1.1 200 OK\r\n";
        http_response += "Content-Type: application/json\r\n";
        http_response += "Content-Length: " + std::to_string(response_json.length()) + "\r\n";
        http_response += "\r\n";
        http_response += response_json;

        send(client_socket, http_response.c_str(), http_response.length(), 0);
        std::cout << "📤 发送响应: " << response_json << std::endl;

        close(client_socket);
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ 处理连接异常: " << e.what() << std::endl;
        close(client_socket);
    }
}

ApiResponse ApiServer::process_request(const std::string &request_json, const std::string &path, const std::string &auth_header)
{
    ApiResponse response;

    try
    {
        // 登录接口（公开）
        if (path == "/api/auth")
        {
            nlohmann::json request_data = nlohmann::json::parse(request_json);
            std::string username = request_data.value("username", "");
            std::string password = request_data.value("password", "");
            // 从配置获取管理员账号（优先使用 config/db_config.json 中的 auth）
            ConfigManager cfg;
            cfg.load_config();
            auto auth = cfg.get_auth_config();

            if (username == auth.admin_user && password == auth.admin_pass)
            {
                // 颁发短期 access token 和长期 refresh token
                int access_exp = 60 * 60;           // 60 分钟
                int refresh_exp = 7 * 24 * 60 * 60; // 7 天
                std::string access_token = jwt::GenerateToken(username, access_exp);

                RefreshTokenStore store;
                std::string refresh_token = store.CreateRefreshToken(username, refresh_exp);

                response.success = true;
                response.message = "登录成功";
                response.data["access_token"] = access_token;
                response.data["expires_in"] = access_exp;
                response.data["refresh_token"] = refresh_token;
                response.data["refresh_expires_in"] = refresh_exp;
            }
            else
            {
                response.success = false;
                response.message = "用户名或密码错误";
                response.error = "Unauthorized";
            }

            return response;
        }

        // 刷新 access token，使用 refresh token 获取新的 access token
        if (path == "/api/auth/refresh")
        {
            nlohmann::json request_data = nlohmann::json::parse(request_json);
            std::string refresh_token = request_data.value("refresh_token", "");
            if (refresh_token.empty())
            {
                response.success = false;
                response.message = "缺少 refresh_token";
                response.error = "Unauthorized";
                return response;
            }

            RefreshTokenStore store;
            std::string sub;
            if (!store.VerifyRefreshToken(refresh_token, sub))
            {
                response.success = false;
                response.message = "无效或已过期的 refresh_token";
                response.error = "Unauthorized";
                return response;
            }

            // 轮换 refresh token：撤销旧 token，签发新 token
            store.RevokeToken(refresh_token);
            int new_refresh_exp = 7 * 24 * 60 * 60;
            std::string new_refresh_token = store.CreateRefreshToken(sub, new_refresh_exp);

            int access_exp = 15 * 60; // 新的短期 access token
            std::string access_token = jwt::GenerateToken(sub, access_exp);

            response.success = true;
            response.message = "刷新成功";
            response.data["access_token"] = access_token;
            response.data["expires_in"] = access_exp;
            response.data["refresh_token"] = new_refresh_token;
            response.data["refresh_expires_in"] = new_refresh_exp;

            return response;
        }

        // 对需要认证的接口进行 token 验证（/api/analyze /api/query /api/batch_analyze /api/excel_analyze）
        // if (path == "/api/status" || path == "/api/query" || path == "/api/analyze" || path == "/api/batch_analyze" || path == "/api/excel_analyze")
        // {
        //     if (auth_header.empty())
        //     {
        //         response.success = false;
        //         response.message = "未提供 Authorization 头";
        //         response.error = "Unauthorized";
        //         return response;
        //     }

        //     std::string token = auth_header;
        //     // 支持直接传入 "Bearer <token>" 或者仅传 token
        //     if (token.rfind("Bearer ", 0) == 0)
        //     {
        //         token = token.substr(7);
        //     }

        //     nlohmann::json claims;
        //     if (!jwt::VerifyToken(token, claims))
        //     {
        //         response.success = false;
        //         response.message = "无效或已过期的 token";
        //         response.error = "Unauthorized";
        //         return response;
        //     }
        // }
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
            if (!request_data.contains("media_type"))
            {
                response.success = false;
                response.message = "请求缺少必要字段: media_type";
                response.error = "Invalid request format";
                return response;
            }

            std::string media_type = request_data["media_type"].get<std::string>();

            ApiRequest request;
            request.media_type = request_data["media_type"].get<std::string>();

            // 根据媒体类型设置请求参数
            if (media_type == "image" || media_type == "video")
            {
                // 处理多个URL的情况，只取第一个
                if (!request_data.contains("media_url"))
                {
                    response.success = false;
                    response.message = "媒体类型为image或video时，必须提供media_url";
                    response.error = "Invalid request format";
                    return response;
                }

                std::string media_url = request_data["media_url"].get<std::string>();
                size_t comma_pos = media_url.find(",");
                if (comma_pos != std::string::npos)
                {
                    media_url = media_url.substr(0, comma_pos);
                    std::cout << "🔍 [信息] 检测到多个URL，只使用第一个: " << media_url << std::endl;
                }
                request.media_url = media_url;
            }
            else if (media_type == "text")
            {
                // 文本类型
                if (!request_data.contains("text"))
                {
                    response.success = false;
                    response.message = "媒体类型为text时，必须提供text";
                    response.error = "Invalid request format";
                    return response;
                }
                request.text = request_data["text"].get<std::string>();
            }
            else if (media_type == "file")
            {
                // 文件类型
                if (!request_data.contains("file_path"))
                {
                    response.success = false;
                    response.message = "媒体类型为file时，必须提供file_path";
                    response.error = "Invalid request format";
                    return response;
                }
                request.file_path = request_data["file_path"].get<std::string>();
            }
            else if (media_type == "audio")
            {
                // 音频类型
                if (!request_data.contains("media_url") && !request_data.contains("file_path"))
                {
                    response.success = false;
                    response.message = "媒体类型为audio时，必须提供media_url或file_path";
                    response.error = "Invalid request format";
                    return response;
                }

                if (request_data.contains("media_url"))
                {
                    std::string media_url = request_data["media_url"].get<std::string>();
                    request.media_url = media_url;
                }

                if (request_data.contains("file_path"))
                {
                    request.file_path = request_data["file_path"].get<std::string>();
                }
            }
            else
            {
                response.success = false;
                response.message = "不支持的媒体类型: " + media_type + " (支持的类型: image, video, text, file, audio)";
                response.error = "Invalid media type";
                return response;
            }

            request.prompt = request_data.value("prompt", "");
            request.max_tokens = request_data.value("max_tokens", 1500);
            request.video_frames = request_data.value("video_frames", 5);
            request.save_to_db = request_data.value("save_to_db", true);

            // 添加大模型配置参数 （可选）
            request.model_name = request_data.value("model_name", "");

            // 处理请求
            double start_time = utils::get_current_time();

            if (request.media_type == "image")
            {
                response = handle_image_analysis(request);
            }
            else if (request.media_type == "video")
            {
                response = handle_video_analysis(request);
            }
            else if (request.media_type == "text")
            {
                // 调用文本分析方法
                try
                {
                    AnalysisResult result = analyzer_->analyze_text(
                        request.text,
                        request.prompt.empty() ? "请分析这段文本" : request.prompt,
                        request.max_tokens,
                        request.model_name);

                    if (result.success)
                    {
                        response.success = true;
                        response.message = "文本分析成功";
                        response.data = {
                            {"content", result.content},
                            {"response_time", result.response_time},
                            {"usage", result.usage}};
                    }
                    else
                    {
                        response.success = false;
                        response.message = "文本分析失败";
                        response.error = result.error;
                    }
                }
                catch (const std::exception &e)
                {
                    response.success = false;
                    response.message = "文本分析异常: " + std::string(e.what());
                    response.error = "Text analysis error";
                }
            }
            else if (request.media_type == "file")
            {
                // 调用文件分析方法
                try
                {
                    AnalysisResult result = analyzer_->analyze_file(
                        request.file_path,
                        request.prompt.empty() ? "请分析这个文件" : request.prompt,
                        request.max_tokens,
                        request.model_name);

                    if (result.success)
                    {
                        response.success = true;
                        response.message = "文件分析成功";
                        response.data = {
                            {"content", result.content},
                            {"response_time", result.response_time},
                            {"usage", result.usage}};
                    }
                    else
                    {
                        response.success = false;
                        response.message = "文件分析失败";
                        response.error = result.error;
                    }
                }
                catch (const std::exception &e)
                {
                    response.success = false;
                    response.message = "文件分析异常: " + std::string(e.what());
                    response.error = "File analysis error";
                }
            }
            else if (request.media_type == "audio")
            {
                // 音频分析 - 可以使用文件分析方法处理音频文件
                try
                {
                    std::string audio_path = request.file_path.empty() ? "" : request.file_path;
                    std::string audio_url = request.media_url.empty() ? "" : request.media_url;

                    // 如果是URL，先下载
                    if (!audio_url.empty())
                    {
                        audio_path = "/tmp/api_audio_" + utils::get_current_timestamp() + ".mp3";
                        if (!utils::download_file(audio_url, audio_path))
                        {
                            response.success = false;
                            response.message = "音频下载失败: " + audio_url;
                            response.error = "Audio download failed";
                            return response;
                        }
                    }

                    // 调用文件分析方法
                    AnalysisResult result = analyzer_->analyze_file(
                        audio_path,
                        request.prompt.empty() ? "请分析这段音频" : request.prompt,
                        request.max_tokens,
                        request.model_name);

                    // 如果是下载的临时文件，清理
                    if (!audio_url.empty() && utils::file_exists(audio_path))
                    {
                        std::filesystem::remove(audio_path);
                    }

                    if (result.success)
                    {
                        response.success = true;
                        response.message = "音频分析成功";
                        response.data = {
                            {"content", result.content},
                            {"response_time", result.response_time},
                            {"usage", result.usage}};
                    }
                    else
                    {
                        response.success = false;
                        response.message = "音频分析失败";
                        response.error = result.error;
                    }
                }
                catch (const std::exception &e)
                {
                    response.success = false;
                    response.message = "音频分析异常: " + std::string(e.what());
                    response.error = "Audio analysis error";
                }
            }

            response.response_time = utils::get_current_time() - start_time;
            return response;
        }

        // 处理Excel分析请求
        if (path == "/api/excel_analyze")
        {
            // 解析JSON请求
            nlohmann::json request_data = nlohmann::json::parse(request_json);

            // 检查必要字段
            if (!request_data.contains("excel_path"))
            {
                response.success = false;
                response.message = "请求缺少必要字段: excel_path";
                response.error = "Invalid request format";
                return response;
            }

            ApiExcelRequest excel_request;
            excel_request.excel_path = request_data["excel_path"].get<std::string>();
            excel_request.output_path = request_data.value("output_path", "");
            excel_request.prompt = request_data.value("prompt", "");
            excel_request.max_tokens = request_data.value("max_tokens", 1500);
            excel_request.save_to_db = request_data.value("save_to_db", true);

            // 处理请求
            double start_time = utils::get_current_time();
            response = handle_excel_analysis(excel_request);
            response.response_time = utils::get_current_time() - start_time;
            return response;
        }

        // 处理数据库媒体分析请求
        if (path == "/api/db_media_analyze")
        {
            // 解析JSON请求
            nlohmann::json request_data = nlohmann::json::parse(request_json);

            // 获取请求参数
            std::string prompt = request_data.value("prompt", "");
            int max_tokens = request_data.value("max_tokens", 2000);
            int video_frames = request_data.value("video_frames", 5);
            bool save_to_db = request_data.value("save_to_db", true);
            // 添加大模型配置参数 （可选）
            std::string model_name = request_data.value("model_name", "");
            // 添加分批请求数参数
            int batch_size = request_data.value("batch_size", 10);

            // 处理请求
            double start_time = utils::get_current_time();
            response = handle_db_media_analysis(prompt, max_tokens, video_frames, save_to_db, model_name, batch_size);
            response.response_time = utils::get_current_time() - start_time;
            return response;
        }

        // 处理批量分析请求
        if (path == "/api/batch_analyze")
        {
            // 解析JSON请求
            nlohmann::json request_data = nlohmann::json::parse(request_json);

            // 检查必要字段
            if (!request_data.contains("requests") || !request_data["requests"].is_array())
            {
                response.success = false;
                response.message = "请求缺少必要字段: requests (必须是数组)";
                response.error = "Invalid request format";
                return response;
            }

            std::vector<ApiRequest> requests;
            const auto &requests_array = request_data["requests"];

            for (const auto &req_json : requests_array)
            {
                if (!req_json.contains("media_type") || !req_json.contains("media_url"))
                {
                    response.success = false;
                    response.message = "批量请求中的某个项目缺少必要字段: media_type 和 media_url";
                    response.error = "Invalid request format";
                    return response;
                }

                ApiRequest req;
                req.media_type = req_json["media_type"].get<std::string>();

                // 处理多个URL的情况，只取第一个
                // req.media_url = req_json["media_url"].get<std::string>();
                std::string media_url = req_json["media_url"].get<std::string>();
                size_t comma_pos = media_url.find(",");
                if (comma_pos != std::string::npos)
                {
                    media_url = media_url.substr(0, comma_pos);
                    std::cout << "🔍 [信息] 检测到多个URL，只使用第一个: " << media_url << std::endl;
                }
                req.media_url = media_url;

                req.prompt = req_json.value("prompt", "");
                req.max_tokens = req_json.value("max_tokens", 1500);
                req.video_frames = req_json.value("video_frames", 5);
                req.save_to_db = req_json.value("save_to_db", true);
                // 添加大模型配置参数 （可选）
                req.model_name = req_json.value("model_name", "");
                // 验证媒体类型
                if (req.media_type != "image" && req.media_type != "video")
                {
                    response.success = false;
                    response.message = "不支持的媒体类型: " + req.media_type + " (必须是 image 或 video)";
                    response.error = "Invalid media type";
                    return response;
                }

                requests.push_back(req);
            }

            // 处理批量分析请求
            double start_time = utils::get_current_time();
            response = handle_batch_analysis(requests);
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
        std::string temp_file = "/tmp/api_image_" + utils::get_current_timestamp() + ".jpg";
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
            request.max_tokens,
            request.model_name);

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
            "keyframes",          // 使用关键帧提取方法
            request.video_frames, // 传递请求的帧数
            request.model_name);

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

// 处理Excel文件分析请求
ApiResponse ApiServer::handle_excel_analysis(const ApiExcelRequest &request)
{
    ApiResponse response;
    double start_time = utils::get_current_time();

    std::cout << "🔄 [Excel分析] 开始处理Excel文件: " << request.excel_path << std::endl;
    std::cout << "⏰ [时间戳] 请求接收时间: " << utils::get_formatted_timestamp() << std::endl;

    try
    {
        // 检查文件是否存在
        if (!std::filesystem::exists(request.excel_path))
        {
            response.success = false;
            response.message = "Excel文件不存在: " + request.excel_path;
            response.error = "File not found";
            return response;
        }

        // 创建Excel处理器
        ExcelProcessor processor;

        // 读取Excel文件
        auto excel_data = processor.read_excel_file(request.excel_path);
        if (excel_data.empty())
        {
            response.success = false;
            response.message = "Excel文件为空或格式不正确";
            response.error = "Empty or invalid Excel file";
            return response;
        }

        // 创建分析任务
        std::string prompt = request.prompt.empty() ? get_image_prompt() : request.prompt;
        int max_tokens = request.max_tokens > 0 ? request.max_tokens : config::DEFAULT_MAX_TOKENS;

        auto tasks = processor.create_analysis_tasks(excel_data, prompt, max_tokens, request.save_to_db);
        if (tasks.empty())
        {
            response.success = false;
            response.message = "没有有效的分析任务";
            response.error = "No valid analysis tasks";
            return response;
        }

        // 添加任务到队列并获取future列表
        auto futures = TaskManager::getInstance().addTasks(tasks);

        // 等待所有任务完成
        std::vector<TaskResult> results;
        results.reserve(futures.size());

        // 创建用于存储分析结果的向量
        std::vector<AnalysisResult> results_db;
        results_db.reserve(futures.size());

        // 创建用于存储分析结果的向量
        std::vector<std::pair<std::string, std::string>> file_id_tags;

        // 等待所有任务完成
        for (auto &future : futures)
        {
            TaskResult result = future.get();
            results.push_back(result);

            // 将分析结果添加到results_db
            if (result.success)
            {
                results_db.push_back(result.result);
            }

            // 如果分析成功，提取标签
            if (result.success && result.result.raw_response.contains("file_id"))
            {
                std::string file_id = result.result.raw_response["file_id"];
                std::string tags;

                // 从分析结果中提取标签
                if (result.result.raw_response.contains("tags"))
                {
                    tags = result.result.raw_response["tags"];
                }
                else
                {
                    // 尝试从内容中提取标签
                    auto extracted_tags = analyzer_->extract_tags(result.result.content);
                    if (!extracted_tags.empty())
                    {
                        nlohmann::json tags_json = extracted_tags;
                        tags = tags_json.dump();
                    }
                }

                if (!tags.empty())
                {
                    file_id_tags.push_back({file_id, tags});
                }
            }
        }

        // 批量请求返回结果保存到数据库

        save_batch_to_database(results_db);

        // // 更新Excel文件
        // if (!file_id_tags.empty() && !request.output_path.empty())
        // {
        //     bool update_success = processor.update_excel_tags(
        //         request.excel_path,
        //         request.output_path,
        //         file_id_tags);

        //     if (!update_success)
        //     {
        //         response.success = false;
        //         response.message = "更新Excel文件失败";
        //         response.error = "Failed to update Excel file";
        //         return response;
        //     }
        // }

        // 准备响应
        response.success = true;
        response.message = "Excel文件处理完成，共分析 " + std::to_string(results.size()) + " 个媒体文件";
        response.data["total_tasks"] = results.size();
        response.data["successful_tasks"] = std::count_if(results.begin(), results.end(),
                                                          [](const TaskResult &r)
                                                          { return r.success; });
        response.data["failed_tasks"] = std::count_if(results.begin(), results.end(),
                                                      [](const TaskResult &r)
                                                      { return !r.success; });
        response.data["output_path"] = request.output_path;

        // 添加详细结果
        nlohmann::json results_json = nlohmann::json::array();
        for (const auto &result : results)
        {
            nlohmann::json result_json;
            result_json["task_id"] = result.task_id;
            result_json["success"] = result.success;
            result_json["file_id"] = result.result.raw_response.contains("file_id") ? result.result.raw_response["file_id"] : "";
            result_json["media_url"] = result.result.raw_response.contains("path") ? result.result.raw_response["path"] : "";
            result_json["media_type"] = result.result.raw_response.contains("type") ? result.result.raw_response["type"] : "";

            if (!result.success)
            {
                result_json["error"] = result.error;
            }
            else
            {
                result_json["content"] = result.result.content;
                result_json["response_time"] = result.result.response_time;

                // 添加标签
                if (result.result.raw_response.contains("tags"))
                {
                    result_json["tags"] = result.result.raw_response["tags"];
                }
                else
                {
                    auto extracted_tags = analyzer_->extract_tags(result.result.content);
                    result_json["tags"] = extracted_tags;
                }
            }

            results_json.push_back(result_json);
        }

        response.data["results"] = results_json;
    }
    catch (const std::exception &e)
    {
        response.success = false;
        response.message = "Excel处理失败: " + std::string(e.what());
        response.error = "Excel processing error";
    }

    response.response_time = utils::get_current_time() - start_time;
    std::cout << "✅ [Excel分析] 处理完成，耗时: " << response.response_time << " 秒" << std::endl;

    return response;
}

// 处理批量分析请求
ApiResponse ApiServer::handle_batch_analysis(const std::vector<ApiRequest> &requests)
{
    ApiResponse response;
    nlohmann::json timing_info = nlohmann::json::object();
    double total_start_time = utils::get_current_time();

    std::cout << "🔄 [批量分析] 开始处理 " << requests.size() << " 个媒体文件" << std::endl;
    std::cout << "⏰ [时间戳] 请求接收时间: " << utils::get_formatted_timestamp() << std::endl;

    try
    {
        // 分批次处理请求，每批5个（减小批次大小以降低内存压力）
        const size_t batch_size = 5;
        size_t total_batches = (requests.size() + batch_size - 1) / batch_size;

        std::cout << "🔄 [批次处理] 准备分 " << total_batches << " 批次处理 " << requests.size() << " 个请求，每批次最多 " << batch_size << " 个" << std::endl;

        // 用于存储所有任务结果
        std::vector<TaskResult> all_results;

        // 分批次处理
        for (size_t i = 0; i < total_batches; ++i)
        {
            size_t start_idx = i * batch_size;
            size_t end_idx = std::min(start_idx + batch_size, requests.size());

            std::cout << "🔍 [批次处理] 正在处理第 " << (i + 1) << "/" << total_batches << " 批次，包含 " << (end_idx - start_idx) << " 个请求" << std::endl;

            // 创建当前批次的任务列表
            std::vector<AnalysisTask> batch_tasks;
            batch_tasks.reserve(end_idx - start_idx);

            for (size_t j = start_idx; j < end_idx; ++j)
            {
                const auto &req = requests[j];

                AnalysisTask task;
                task.id = "batch_" + std::to_string(j) + "_" + utils::get_current_timestamp();
                task.media_url = req.media_url;
                task.media_type = req.media_type;
                // 大模型
                task.model_name = req.model_name;
                //
                task.prompt = req.prompt.empty() ? (req.media_type == "video" ? get_video_prompt() : get_image_prompt()) : req.prompt;
                task.max_tokens = req.max_tokens > 0 ? req.max_tokens : config::DEFAULT_MAX_TOKENS;
                task.video_frames = req.video_frames > 0 ? req.video_frames : config::DEFAULT_VIDEO_FRAMES;
                task.save_to_db = req.save_to_db;

                batch_tasks.push_back(task);
            }

            // 添加当前批次任务到队列并获取future列表
            auto futures = TaskManager::getInstance().addTasks(batch_tasks);

            // 用于存储当前批次的结果
            std::vector<AnalysisResult> batch_results;

            // 等待当前批次的所有任务完成
            for (auto &future : futures)
            {
                TaskResult taskResult = future.get();
                all_results.push_back(taskResult);

                // 将成功的分析结果添加到当前批次结果
                if (taskResult.success)
                {
                    batch_results.push_back(taskResult.result);
                }
            }

            // 直接保存当前批次的结果到数据库
            if (!batch_results.empty())
            {
                std::cout << "💾 [数据库保存] 正在保存第 " << (i + 1) << "/" << total_batches << " 批次结果，包含 " << batch_results.size() << " 条记录" << std::endl;

                if (!save_batch_to_database(batch_results))
                {
                    std::cerr << "❌ [数据库保存] 第 " << (i + 1) << " 批次保存失败" << std::endl;
                }
                else
                {
                    std::cout << "✅ [数据库保存] 第 " << (i + 1) << " 批次保存成功" << std::endl;
                }
            }

            std::cout << "✅ [批次处理] 第 " << (i + 1) << " 批次处理完成" << std::endl;
        }

        std::cout << "🎉 [批次处理] 所有批次处理完成，共处理 " << all_results.size() << " 个任务" << std::endl;

        // 构建响应数据
        nlohmann::json results_array = nlohmann::json::array();
        int success_count = 0;

        for (const auto &result : all_results)
        {
            nlohmann::json result_obj;
            result_obj["task_id"] = result.task_id;
            result_obj["success"] = result.success;

            if (result.success)
            {
                result_obj["content"] = result.result.content;
                result_obj["tags"] = utils::extract_tags(result.result.content);
                result_obj["response_time"] = result.result.response_time;
                result_obj["usage"] = result.result.usage;
                success_count++;
            }
            else
            {
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
            {"failed", requests.size() - success_count}};

        double total_time = utils::get_current_time() - total_start_time;
        timing_info["total_seconds"] = total_time;
        timing_info["pending_tasks"] = TaskManager::getInstance().getPendingTaskCount();
        timing_info["active_threads"] = TaskManager::getInstance().getActiveThreadCount();

        std::cout << "✅ [批量分析] 处理完成，成功: " << success_count << "/" << requests.size() << std::endl;
        std::cout << "⏰ [时间戳] 请求处理完成时间: " << utils::get_formatted_timestamp() << std::endl;
        std::cout << "🎉 [完成] 批量分析请求处理完成，总耗时: " << total_time << " 秒" << std::endl;
    }
    catch (const std::exception &e)
    {
        response.success = false;
        response.message = "批量分析失败: " + std::string(e.what());
        response.error = "Batch analysis error";

        std::cerr << "❌ [批量分析] 异常: " << e.what() << std::endl;
    }

    response.data["timing"] = timing_info;
    response.response_time = utils::get_current_time() - total_start_time;

    return response;
}
// 批量保存分析结果到数据库（异步处理）
bool ApiServer::save_batch_to_database(const std::vector<AnalysisResult> &results)
{
    try
    {
        // 创建异步任务来保存到数据库，避免阻塞主循环
        std::thread db_thread([this, results]()
                              {
            try {
                bool success = analyzer_->save_batch_results_to_database(results);
                if (!success) {
                    std::cerr << "❌ 异步保存到数据库失败" << std::endl;
                } else {
                    std::cout << "✅ 异步保存到数据库成功，共 " << results.size() << " 条记录" << std::endl;
                }
            } catch (const std::exception &e) {
                std::cerr << "❌ 异步保存到数据库异常: " << e.what() << std::endl;
            } });

        // 分离线程，使其在后台运行
        db_thread.detach();

        // 立即返回true，表示异步任务已启动
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ 启动异步数据库保存任务失败: " << e.what() << std::endl;
        return false;
    }
}

// 处理数据库媒体分析请求
ApiResponse ApiServer::handle_db_media_analysis(const std::string &prompt, int max_tokens, int video_frames, bool save_to_db, const std::string &model_name, int batch_size)
{
    ApiResponse response;
    double start_time = utils::get_current_time();

    std::cout << "🔄 [数据库媒体分析] 开始处理数据库中的媒体" << std::endl;
    std::cout << "⏰ [时间戳] 请求接收时间: " << utils::get_formatted_timestamp() << std::endl;

    try
    {
        // 创建Excel处理器
        ExcelProcessor processor;

        // 从数据库读取媒体数据
        auto media_data = processor.read_media_from_db();
        if (media_data.empty())
        {
            response.success = false;
            response.message = "数据库中没有媒体数据";
            response.error = "No media data in database";
            return response;
        }

        std::cout << "📊 [数据库媒体分析] 从数据库读取到 " << media_data.size() << " 条媒体数据" << std::endl;

        // 创建分析任务
        std::string analysis_prompt = prompt.empty() ? get_image_prompt() : prompt;
        int tokens = max_tokens > 0 ? max_tokens : config::DEFAULT_MAX_TOKENS;

        // 分批次处理数据，使用传入的batch_size参数，默认为5（减小批次大小以降低内存压力）
        const size_t actual_batch_size = batch_size > 0 ? std::min((size_t)batch_size, (size_t)5) : 5;
        size_t total_batches = (media_data.size() + actual_batch_size - 1) / actual_batch_size;

        std::cout << "🔄 [批次处理] 准备分 " << total_batches << " 批次处理数据，每批次最多 " << actual_batch_size << " 条" << std::endl;

        // 用于存储所有任务结果
        std::vector<TaskResult> all_results;

        // 分批次处理
        for (size_t i = 0; i < total_batches; ++i)
        {
            size_t start_idx = i * actual_batch_size;
            size_t end_idx = std::min(start_idx + actual_batch_size, media_data.size());

            std::vector<ExcelRowData> batch_data(media_data.begin() + start_idx, media_data.begin() + end_idx);

            std::cout << "🔍 [批次处理] 正在处理第 " << (i + 1) << "/" << total_batches << " 批次，包含 " << batch_data.size() << " 条数据" << std::endl;

            // 为当前批次创建分析任务
            auto tasks = processor.create_analysis_tasks(batch_data, analysis_prompt, tokens, video_frames, save_to_db, model_name);
            if (tasks.empty())
            {
                std::cout << "⚠️ [批次处理] 第 " << (i + 1) << " 批次没有有效的分析任务，跳过" << std::endl;
                continue;
            }

            // 添加任务到队列并获取future列表
            auto futures = TaskManager::getInstance().addTasks(tasks);

            // 用于存储当前批次的结果
            std::vector<AnalysisResult> batch_results;

            // 等待当前批次的所有任务完成
            for (auto &future : futures)
            {
                TaskResult result = future.get();
                all_results.push_back(result);

                // 将成功的分析结果添加到当前批次结果
                if (result.success)
                {
                    batch_results.push_back(result.result);
                }
            }

            // 直接保存当前批次的结果到数据库
            if (save_to_db && !batch_results.empty())
            {
                std::cout << "💾 [数据库保存] 正在保存第 " << (i + 1) << "/" << total_batches << " 批次结果，包含 " << batch_results.size() << " 条记录" << std::endl;

                if (!save_batch_to_database(batch_results))
                {
                    std::cerr << "❌ [数据库保存] 第 " << (i + 1) << " 批次保存失败" << std::endl;
                }
                else
                {
                    std::cout << "✅ [数据库保存] 第 " << (i + 1) << " 批次保存成功" << std::endl;
                }
            }

            std::cout << "✅ [批次处理] 第 " << (i + 1) << " 批次处理完成" << std::endl;
        }

        std::cout << "🎉 [批次处理] 所有批次处理完成，共处理 " << all_results.size() << " 个任务" << std::endl;

        // 准备响应
        response.success = true;
        response.message = "数据库媒体处理完成，共分析 " + std::to_string(all_results.size()) + " 个媒体文件";
        response.data["total_tasks"] = all_results.size();
        response.data["successful_tasks"] = std::count_if(all_results.begin(), all_results.end(),
                                                          [](const TaskResult &r)
                                                          { return r.success; });
        response.data["failed_tasks"] = std::count_if(all_results.begin(), all_results.end(),
                                                      [](const TaskResult &r)
                                                      { return !r.success; });

        // 添加详细结果
        nlohmann::json results_json = nlohmann::json::array();
        for (const auto &result : all_results)
        {
            nlohmann::json result_json;
            result_json["task_id"] = result.task_id;
            result_json["success"] = result.success;
            result_json["file_id"] = result.result.raw_response.contains("file_id") ? result.result.raw_response["file_id"] : "";
            result_json["media_url"] = result.result.raw_response.contains("path") ? result.result.raw_response["path"] : "";
            result_json["media_type"] = result.result.raw_response.contains("type") ? result.result.raw_response["type"] : "";

            if (!result.success)
            {
                result_json["error"] = result.error;
            }
            else
            {
                result_json["content"] = result.result.content;
                result_json["response_time"] = result.result.response_time;

                // 添加标签
                if (result.result.raw_response.contains("tags"))
                {
                    result_json["tags"] = result.result.raw_response["tags"];
                }
                else
                {
                    auto extracted_tags = analyzer_->extract_tags(result.result.content);
                    result_json["tags"] = extracted_tags;
                }
            }

            results_json.push_back(result_json);
        }

        response.data["results"] = results_json;
        response.response_time = utils::get_current_time() - start_time;
    }
    catch (const std::exception &e)
    {
        response.success = false;
        response.message = "处理数据库媒体时发生错误: " + std::string(e.what());
        response.error = std::string(e.what());
        response.response_time = utils::get_current_time() - start_time;
    }

    return response;
}
