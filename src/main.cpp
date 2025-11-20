#include "DoubaoMediaAnalyzer.hpp"
#include "utils.hpp"
#include "config.hpp"
#include "GPUManager.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <filesystem>

// 提示词函数
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

std::string get_image_prompt()
{
    return R"(请仔细观察图片内容，为图片生成合适的标签。要求：
1. 仔细观察图片的各个细节
2. 生成的标签要准确反映图片的主题、场景、动作等
3. 请严格按照以下多级标签体系对进行分类：
 一级标签：选择最概括的主类别。
 二级标签：在一级标签下选择更具体的子类别。
 三级标签：在二级标签下选择最精准的描述性标签
 四级标签：在三级标签下选择最精准的描述性标签
 五级标签：在四级标签下选择最精准的描述性标签
4. 输出格式：通过分析，生成的标签为：['一级标签', '二级标签', '三级标签', '四级标签', '五级标签'])";
}

std::string get_video_prompt()
{
    return R"(请仔细观察视频的关键帧内容，为视频生成合适的标签。要求：
1. 综合分析视频的整体内容和关键帧
2. 生成的标签要准确反映视频的主题、场景、动作等
3. 请严格按照以下三级标签体系对视频进行分类：
 一级标签：选择最概括的主类别。
 二级标签：在一级标签下选择更具体的子类别。
 三级标签：在二级标签下选择最精准的描述性标签
4. 输出格式：通过分析视频，生成的标签为：['标签1', '标签2', '标签3'])";
}

void print_usage()
{
    std::cout << "用法: doubao_analyzer [选项]" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --api-key KEY        API密钥 (必需)" << std::endl;
    std::cout << "  --base-url URL       API基础URL (可选，默认使用配置文件中的URL)" << std::endl;
    std::cout << "  --model-name NAME    模型名称 (可选，默认使用配置文件中的模型)" << std::endl;
    std::cout << "  --image PATH         单张图片路径" << std::endl;
    std::cout << "  --video PATH         单个视频路径" << std::endl;
    std::cout << "  --folder PATH        媒体文件夹路径" << std::endl;
    std::cout << "  --file-type TYPE     分析的文件类型 [all|image|video] (默认: all)" << std::endl;
    std::cout << "  --prompt TEXT        自定义提示词" << std::endl;
    std::cout << "  --max-files NUM      最大分析文件数量 (默认: 5)" << std::endl;
    std::cout << "  --video-frames NUM   视频提取帧数 (默认: 5)" << std::endl;
    std::cout << "  --output PATH        结果保存路径" << std::endl;
    std::cout << "  --save-to-db        将结果保存到数据库" << std::endl;
    std::cout << "  --query-db CONDITION 查询数据库记录" << std::endl;
    std::cout << "  --query-tag TAG      按标签查询数据库记录" << std::endl;
    std::cout << "  --db-stats           显示数据库统计信息" << std::endl;
    std::cout << "  --help               显示此帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  doubao_analyzer --api-key YOUR_KEY --image test.jpg" << std::endl;
    std::cout << "  doubao_analyzer --api-key YOUR_KEY --video test.mp4 --video-frames 8" << std::endl;
    std::cout << "  doubao_analyzer --api-key YOUR_KEY --folder ./media --file-type all" << std::endl;
}

void print_result(const AnalysisResult &result, const std::string &media_type)
{
    if (result.success)
    {
        std::cout << "✅ " << media_type << "分析成功!" << std::endl;
        std::cout << "⏱️  响应时间: " << result.response_time << "秒" << std::endl;
        std::cout << "📝 分析结果:" << std::endl
                  << result.content << std::endl;

        auto tags = utils::extract_tags(result.content);
        if (!tags.empty())
        {
            std::cout << "🏷️  提取标签: ";
            for (size_t i = 0; i < tags.size(); ++i)
            {
                if (i > 0)
                    std::cout << ", ";
                std::cout << tags[i];
            }
            std::cout << std::endl;
        }
    }
    else
    {
        std::cout << "❌ " << media_type << "分析失败: " << result.error << std::endl;
    }
}

void print_statistics(const std::vector<AnalysisResult> &results)
{
    int success_count = 0;
    int total_count = results.size();
    int video_count = 0;
    int image_count = 0;

    double total_time = 0;
    double video_total_time = 0;
    double image_total_time = 0;
    int video_success_count = 0;
    int image_success_count = 0;

    for (const auto &result : results)
    {
        if (result.success)
        {
            success_count++;
            total_time += result.response_time;
        }

        if (result.raw_response.contains("type"))
        {
            std::string type = result.raw_response["type"];
            if (type == "video")
            {
                video_count++;
                if (result.success)
                {
                    video_total_time += result.response_time;
                    video_success_count++;
                }
            }
            else if (type == "image")
            {
                image_count++;
                if (result.success)
                {
                    image_total_time += result.response_time;
                    image_success_count++;
                }
            }
        }
    }

    std::cout << "\n📊 分析统计:" << std::endl;
    std::cout << "   总文件数: " << total_count << std::endl;
    std::cout << "   成功分析: " << success_count << "/" << total_count << std::endl;
    std::cout << "   图片文件: " << image_count << std::endl;
    std::cout << "   视频文件: " << video_count << std::endl;

    if (success_count > 0)
    {
        double avg_time = total_time / success_count;
        std::cout << "⏱️  平均响应时间: " << avg_time << "秒" << std::endl;

        if (image_success_count > 0)
        {
            double avg_image_time = image_total_time / image_success_count;
            std::cout << "   图片平均时间: " << avg_image_time << "秒" << std::endl;
        }

        if (video_success_count > 0)
        {
            double avg_video_time = video_total_time / video_success_count;
            std::cout << "   视频平均时间: " << avg_video_time << "秒" << std::endl;
        }
    }
}

void interactive_mode()
{
    std::string api_key;
    std::cout << "请输入API密钥: ";
    std::getline(std::cin, api_key);

    if (api_key.empty())
    {
        std::cout << "❌ API密钥不能为空" << std::endl;
        return;
    }

    std::string use_custom;
    std::cout << "是否使用自定义API配置? (y/n, 默认n): ";
    std::getline(std::cin, use_custom);

    std::unique_ptr<DoubaoMediaAnalyzer> analyzer;

    if (use_custom == "y" || use_custom == "Y")
    {
        std::string base_url;
        std::cout << "请输入API基础URL: ";
        std::getline(std::cin, base_url);

        std::string model_name;
        std::cout << "请输入模型名称: ";
        std::getline(std::cin, model_name);

        analyzer = std::make_unique<DoubaoMediaAnalyzer>(api_key, base_url, model_name);
        std::cout << "🔧 使用自定义API配置" << std::endl;
    }
    else
    {
        analyzer = std::make_unique<DoubaoMediaAnalyzer>(api_key);
        std::cout << "🔧 使用默认API配置" << std::endl;
    }

    if (!analyzer->test_connection())
    {
        return;
    }

    while (true)
    {
        std::cout << "\n"
                  << std::string(50, '=') << std::endl;
        std::cout << "1. 分析单张图片" << std::endl;
        std::cout << "2. 分析单个视频" << std::endl;
        std::cout << "3. 批量分析文件夹" << std::endl;
        std::cout << "4. 测试API连接" << std::endl;
        std::cout << "5. 退出" << std::endl;

        std::string choice;
        std::cout << "请选择操作 (1-5): ";
        std::getline(std::cin, choice);

        if (choice == "1")
        {
            std::string image_path;
            std::cout << "请输入图片路径(直接回车使用默认./test/test.jpg): ";
            std::getline(std::cin, image_path);
            if (image_path.empty())
            {
                image_path = "./test/test.jpg";
            }

            if (utils::file_exists(image_path))
            {
                std::string prompt;
                std::cout << "请输入提示词 (直接回车使用默认): ";
                std::getline(std::cin, prompt);
                if (prompt.empty())
                {
                    prompt = get_image_prompt();
                }

                auto result = analyzer->analyze_single_image(image_path, prompt);
                print_result(result, "图片");
            }
            else
            {
                std::cout << "❌ 图片文件不存在" << std::endl;
            }
        }
        else if (choice == "2")
        {
            std::string video_path;
            std::cout << "请输入视频路径(直接回车使用默认./test/test.mp4): ";
            std::getline(std::cin, video_path);
            if (video_path.empty())
            {
                video_path = "./test/test.mp4";
            }

            if (utils::file_exists(video_path))
            {
                std::string prompt;
                std::cout << "请输入提示词 (直接回车使用默认): ";
                std::getline(std::cin, prompt);
                if (prompt.empty())
                {
                    prompt = get_video_prompt();
                }

                std::string frames_input;
                std::cout << "提取帧数 (默认5): ";
                std::getline(std::cin, frames_input);
                int num_frames = frames_input.empty() ? 5 : std::stoi(frames_input);

                std::cout << "🎬 开始分析视频..." << std::endl;
                auto result = analyzer->analyze_single_video(video_path, prompt, 2000, num_frames);
                print_result(result, "视频");
            }
            else
            {
                std::cout << "❌ 视频文件不存在" << std::endl;
            }
        }
        else if (choice == "3")
        {
            std::string folder_path;
            std::cout << "请输入媒体文件夹路径: ";
            std::getline(std::cin, folder_path);

            if (utils::file_exists(folder_path))
            {
                std::cout << "选择分析类型:" << std::endl;
                std::cout << "1. 所有文件 (图片+视频)" << std::endl;
                std::cout << "2. 仅图片" << std::endl;
                std::cout << "3. 仅视频" << std::endl;

                std::string type_choice;
                std::cout << "请选择 (1-3, 默认1): ";
                std::getline(std::cin, type_choice);

                std::string file_type = "all";
                if (type_choice == "2")
                    file_type = "image";
                else if (type_choice == "3")
                    file_type = "video";

                std::string max_files_input;
                std::cout << "最大分析数量 (默认5): ";
                std::getline(std::cin, max_files_input);
                int max_files = max_files_input.empty() ? 5 : std::stoi(max_files_input);

                std::string prompt;
                std::cout << "请输入提示词 (直接回车使用默认): ";
                std::getline(std::cin, prompt);
                if (prompt.empty())
                {
                    prompt = (file_type == "video") ? get_video_prompt() : get_image_prompt();
                }

                auto results = analyzer->batch_analyze(folder_path, prompt, max_files, file_type);
                print_statistics(results);
            }
            else
            {
                std::cout << "❌ 文件夹不存在" << std::endl;
            }
        }
        else if (choice == "4")
        {
            analyzer->test_connection();
        }
        else if (choice == "5")
        {
            std::cout << "👋 再见!" << std::endl;
            break;
        }
        else
        {
            std::cout << "❌ 无效选择" << std::endl;
        }
    }
}

int main(int argc, char *argv[])
{
    // 检查命令行参数
    if (argc == 1)
    {
        interactive_mode();
        return 0;
    }

    // 解析命令行参数
    std::string api_key;
    std::string base_url;   // 新增：API基础URL
    std::string model_name; // 新增：模型名称
    std::string image_path;
    std::string video_path;
    std::string folder_path;
    std::string file_type = "all";
    std::string prompt;
    std::string output_path;
    int max_files = 5;
    int video_frames = 5;
    bool save_to_db = false;
    std::string query_db;
    std::string query_tag;
    bool show_db_stats = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--help")
        {
            print_usage();
            return 0;
        }
        else if (arg == "--api-key" && i + 1 < argc)
        {
            api_key = argv[++i];
        }
        else if (arg == "--base-url" && i + 1 < argc)
        {
            base_url = argv[++i];
        }
        else if (arg == "--model-name" && i + 1 < argc)
        {
            model_name = argv[++i];
        }
        else if (arg == "--image" && i + 1 < argc)
        {
            image_path = argv[++i];
        }
        else if (arg == "--video" && i + 1 < argc)
        {
            video_path = argv[++i];
        }
        else if (arg == "--folder" && i + 1 < argc)
        {
            folder_path = argv[++i];
        }
        else if (arg == "--file-type" && i + 1 < argc)
        {
            file_type = argv[++i];
        }
        else if (arg == "--prompt" && i + 1 < argc)
        {
            prompt = argv[++i];
        }
        else if (arg == "--max-files" && i + 1 < argc)
        {
            max_files = std::stoi(argv[++i]);
        }
        else if (arg == "--video-frames" && i + 1 < argc)
        {
            video_frames = std::stoi(argv[++i]);
        }
        else if (arg == "--output" && i + 1 < argc)
        {
            output_path = argv[++i];
        }
        else if (arg == "--save-to-db")
        {
            save_to_db = true;
        }
        else if (arg == "--query-db" && i + 1 < argc)
        {
            query_db = argv[++i];
        }
        else if (arg == "--query-tag" && i + 1 < argc)
        {
            query_tag = argv[++i];
        }
        else if (arg == "--db-stats")
        {
            show_db_stats = true;
        }
    }

    if (api_key.empty())
    {
        std::cout << "❌ 必须提供API密钥，使用 --api-key 参数" << std::endl;
        print_usage();
        return 1;
    }

    // 创建分析器
    DoubaoMediaAnalyzer *analyzer_ptr;

    // 根据提供的参数选择适当的构造函数
    if (!base_url.empty() && !model_name.empty())
    {
        // 使用自定义API配置
        analyzer_ptr = new DoubaoMediaAnalyzer(api_key, base_url, model_name);
        std::cout << "🔧 使用自定义API配置" << std::endl;
        std::cout << "   URL: " << base_url << std::endl;
        std::cout << "   模型: " << model_name << std::endl;
    }
    else
    {
        // 使用默认配置
        analyzer_ptr = new DoubaoMediaAnalyzer(api_key);
        std::cout << "🔧 使用默认API配置" << std::endl;
    }

    // 使用智能指针管理资源
    std::unique_ptr<DoubaoMediaAnalyzer> analyzer(analyzer_ptr);

    std::cout << "🚀 豆包大模型媒体分析调试工具（支持图片和视频）" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // 初始化GPU管理器
    gpu::GPUManager::initialize();

    // 初始化数据库
    if (save_to_db || !query_db.empty() || !query_tag.empty() || show_db_stats)
    {
        std::cout << "🔌 正在初始化数据库连接..." << std::endl;
        if (!analyzer->initialize_database())
        {
            std::cout << "❌ 数据库初始化失败" << std::endl;
            return 1;
        }
        std::cout << "✅ 数据库连接成功" << std::endl;
    }

    // 测试连接
    if (!analyzer->test_connection())
    {
        return 1;
    }

    // 处理数据库查询和统计请求
    if (!query_db.empty())
    {
        std::cout << "🔍 查询数据库记录: " << query_db << std::endl;
        auto db_results = analyzer->query_database_results(query_db);

        if (db_results.empty())
        {
            std::cout << "❌ 未找到匹配的记录" << std::endl;
        }
        else
        {
            std::cout << "✅ 找到 " << db_results.size() << " 条记录:" << std::endl;
            for (const auto &record : db_results)
            {
                std::cout << "📄 文件: " << record.file_name
                          << " (" << record.file_type << ")" << std::endl;
                std::cout << "📅 时间: " << record.created_at << std::endl;
                std::cout << "⏱️  响应时间: " << record.response_time << "秒" << std::endl;
                std::cout << "🏷️  标签: " << record.tags << std::endl;
                std::cout << "📝 分析结果: " << record.analysis_result << std::endl;
            }
        }
        return 0;
    }

    if (!query_tag.empty())
    {
        std::cout << "🏷️  按标签查询数据库记录: " << query_tag << std::endl;
        auto db_results = analyzer->query_by_tag(query_tag);

        if (db_results.empty())
        {
            std::cout << "❌ 未找到包含标签 '" << query_tag << "' 的记录" << std::endl;
        }
        else
        {
            std::cout << "✅ 找到 " << db_results.size() << " 条记录:" << std::endl;
            for (const auto &record : db_results)
            {
                std::cout << "📄 文件: " << record.file_name
                          << " (" << record.file_type << ")" << std::endl;
                std::cout << "📅 时间: " << record.created_at << std::endl;
                std::cout << "⏱️  响应时间: " << record.response_time << "秒" << std::endl;
                std::cout << "🏷️  标签: " << record.tags << std::endl;
                std::cout << "📝 分析结果: " << record.analysis_result << std::endl;
            }
        }
        return 0;
    }

    if (show_db_stats)
    {
        std::cout << "📊 数据库统计信息:" << std::endl;
        auto stats = analyzer->get_database_statistics();

        if (stats.empty())
        {
            std::cout << "❌ 无法获取统计信息" << std::endl;
        }
        else
        {
            if (stats.contains("total_analyses"))
            {
                std::cout << "总分析数量: " << stats["total_analyses"] << std::endl;
            }
            if (stats.contains("image_analyses"))
            {
                std::cout << "图片分析数量: " << stats["image_analyses"] << std::endl;
            }
            if (stats.contains("video_analyses"))
            {
                std::cout << "视频分析数量: " << stats["video_analyses"] << std::endl;
            }
            if (stats.contains("avg_response_time"))
            {
                std::cout << "平均响应时间: " << stats["avg_response_time"] << "秒" << std::endl;
            }
            if (stats.contains("top_tags"))
            {
                std::cout << "最常用标签:" << std::endl;
                for (const auto &tag : stats["top_tags"])
                {
                    std::cout << "  - " << tag["tag"] << ": " << tag["count"] << "次" << std::endl;
                }
            }
        }
        return 0;
    }

    std::vector<AnalysisResult> results;

    // 单张图片分析
    if (!image_path.empty())
    {
        std::cout << "\n📸 分析单张图片: " << image_path << std::endl;
        std::string analysis_prompt = prompt.empty() ? get_image_prompt() : prompt;
        auto result = analyzer->analyze_single_image(image_path, analysis_prompt);
        print_result(result, "图片");

        result.raw_response["file"] = std::filesystem::path(image_path).filename().string();
        result.raw_response["path"] = image_path;
        result.raw_response["type"] = "image";
        results.push_back(result);

        // 保存到数据库
        if (save_to_db)
        {
            analyzer->save_result_to_database(result);
        }
    }

    // 单个视频分析
    if (!video_path.empty())
    {
        std::cout << "\n🎬 分析单个视频: " << video_path << std::endl;
        std::string analysis_prompt = prompt.empty() ? get_video_prompt() : prompt;
        auto result = analyzer->analyze_single_video(video_path, analysis_prompt, 2000, video_frames);
        print_result(result, "视频");

        result.raw_response["file"] = std::filesystem::path(video_path).filename().string();
        result.raw_response["path"] = video_path;
        result.raw_response["type"] = "video";
        results.push_back(result);

        // 保存到数据库
        if (save_to_db)
        {
            analyzer->save_result_to_database(result);
        }
    }

    // 批量媒体分析
    if (!folder_path.empty())
    {
        std::cout << "\n📁 批量分析文件夹: " << folder_path << " (文件类型: " << file_type << ")" << std::endl;
        std::string analysis_prompt = prompt.empty() ? (file_type == "video" ? get_video_prompt() : get_image_prompt()) : prompt;

        auto batch_results = analyzer->batch_analyze(folder_path, analysis_prompt, max_files, file_type);
        results.insert(results.end(), batch_results.begin(), batch_results.end());

        // 保存到数据库
        if (save_to_db)
        {
            analyzer->save_batch_results_to_database(results);
        }
    }

    // 保存结果
    if (!output_path.empty() && !results.empty())
    {
        try
        {
            nlohmann::json output_json = nlohmann::json::array();
            for (const auto &result : results)
            {
                output_json.push_back(result.raw_response);
            }

            std::ofstream file(output_path);
            file << output_json.dump(2) << std::endl;
            std::cout << "\n💾 结果已保存到: " << output_path << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cout << "❌ 保存结果失败: " << e.what() << std::endl;
        }
    }

    // 统计信息
    if (!results.empty())
    {
        print_statistics(results);
    }

    return 0;
}
