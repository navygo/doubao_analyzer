#include "ApiServer.hpp"
#include "utils.hpp"
#include "GPUManager.hpp"
#include <iostream>
#include <string>
#include <signal.h>
#include <memory>

// 全局服务器指针，用于信号处理
std::unique_ptr<ApiServer> g_server = nullptr;

// 信号处理函数
void signal_handler(int signum)
{
    std::cout << "收到信号 " << signum << "，正在关闭服务器..." << std::endl;
    if (g_server)
    {
        g_server->stop();
        exit(0);
    }
}

void print_usage()
{
    std::cout << "用法: doubao_api_server [选项]" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --api-key KEY        豆包API密钥 (必需)" << std::endl;
    std::cout << "  --port PORT          服务器监听端口 (默认: 8080)" << std::endl;
    std::cout << "  --host HOST          服务器绑定地址 (默认: 0.0.0.0)" << std::endl;
    std::cout << "  --help               显示此帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  doubao_api_server --api-key YOUR_KEY --port 8080" << std::endl;
}

int main(int argc, char *argv[])
{
    std::string api_key;
    int port = 8080;
    std::string host = "0.0.0.0";

    // 解析命令行参数
    for (int i = 1; i < argc; i++)
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
        else if (arg == "--port" && i + 1 < argc)
        {
            port = std::stoi(argv[++i]);
        }
        else if (arg == "--host" && i + 1 < argc)
        {
            host = argv[++i];
        }
    }

    // 检查API密钥
    if (api_key.empty())
    {
        std::cout << "❌ 请提供豆包API密钥" << std::endl;
        print_usage();
        return 1;
    }

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 初始化GPU管理器
    gpu::GPUManager::initialize();

    // 创建并初始化API服务器
    g_server = std::make_unique<ApiServer>(api_key, port, host);

    if (!g_server->initialize())
    {
        std::cout << "❌ API服务器初始化失败" << std::endl;
        return 1;
    }

    // 启动服务器
    std::cout << "🚀 启动豆包媒体分析API服务器..." << std::endl;
    g_server->start();

    return 0;
}
