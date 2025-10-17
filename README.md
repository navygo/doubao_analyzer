<<<<<<< HEAD
# 豆包大模型媒体分析工具 (C++17)

基于字节跳动豆包大模型的图片和视频分析工具，使用C++17重写，支持Ubuntu系统部署。

## 功能特性

- 🖼️ **图片分析**: 支持常见图片格式 (JPG, PNG, BMP, WebP等)
- 🎬 **视频分析**: 支持多种视频格式 (MP4, AVI, MOV, MKV等)
- 📁 **批量处理**: 支持文件夹批量分析
- 🏷️ **智能标签**: 自动从分析结果中提取标签
- ⚡ **高性能**: C++17实现，处理速度快
- 🔧 **易部署**: 完整的CMake构建系统

## 系统要求

- Ubuntu 18.04 或更高版本
- C++17 兼容编译器 (GCC 7+)
- CMake 3.10+

## 快速开始

### 1. 安装依赖
```bash
chmod +x install_deps.sh
./install_deps.sh


2. 编译项目
BASH
mkdir build && cd build
cmake ..
make -j$(nproc)
3. 安装到系统
BASH
sudo make install
4. 运行测试
BASH
# 编译测试程序
make test_config

# 运行功能测试
./test_config
使用方法
命令行模式
BASH
# 分析单张图片
doubao_analyzer --api-key YOUR_API_KEY --image test.jpg

# 分析单个视频
doubao_analyzer --api-key YOUR_API_KEY --video test.mp4 --video-frames 8

# 批量分析文件夹
doubao_analyzer --api-key YOUR_API_KEY --folder ./media --file-type all --max-files 10

# 仅分析视频文件
doubao_analyzer --api-key YOUR_API_KEY --folder ./videos --file-type video

# 保存结果到文件
doubao_analyzer --api-key YOUR_API_KEY --folder ./media --output results.json
交互式模式
BASH
doubao_analyzer
API配置
获取豆包API密钥
在命令行或交互式模式中输入密钥
工具会自动测试连接
项目结构
TEXT
doubao_analyzer/
├── CMakeLists.txt          # 构建配置
├── include/               # 头文件
│   ├── DoubaoMediaAnalyzer.hpp
│   ├── utils.hpp
│   └── config.hpp
├── src/                  # 源文件
│   ├── main.cpp
│   ├── DoubaoMediaAnalyzer.cpp
│   └── utils.cpp
├── test/                # 测试文件
│   ├── test_config.cpp
│   ├── test.jpg
│   └── test.mp4
├── setup.sh            # 部署脚本
├── install_deps.sh     # 依赖安装脚本
└── README.md          # 说明文档
开发说明
添加新的媒体格式
在 config.hpp 中扩展对应的文件扩展名数组。

自定义分析提示词
修改 main.cpp 中的 get_image_prompt() 和 get_video_prompt() 函数。

性能调优
调整 config.hpp 中的超时设置
修改视频帧提取数量
调整图像压缩质量
故障排除
常见问题
编译错误: 确保安装了所有依赖
OpenCV找不到: 运行 pkg-config --modversion opencv4
API连接失败: 检查API密钥和网络连接
调试模式
编译时添加调试信息：

BASH
cmake -DCMAKE_BUILD_TYPE=Debug ..
许可证
MIT License

贡献
欢迎提交Issue和Pull Request！

TEXT

## 12. 完整的CMakeLists.txt (更新版)
```cmake
cmake_minimum_required(VERSION 3.10)
project(DoubaoMediaAnalyzer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 查找依赖包
find_package(PkgConfig REQUIRED)

# 查找OpenCV
find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
    message(STATUS "Found OpenCV: ${OpenCV_VERSION}")
    include_directories(${OpenCV_INCLUDE_DIRS})
endif()

# 查找CURL
find_package(CURL REQUIRED)
if(CURL_FOUND)
    message(STATUS "Found CURL: ${CURL_VERSION}")
endif()

# 查找nlohmann_json
find_package(nlohmann_json 3.2.0 REQUIRED)
if(nlohmann_json_FOUND)
    message(STATUS "Found nlohmann_json: ${nlohmann_json_VERSION}")
endif()

# 包含目录
include_directories(include)

# 主程序
set(SOURCES
    src/main.cpp
    src/DoubaoMediaAnalyzer.cpp
    src/utils.cpp
)

add_executable(doubao_analyzer ${SOURCES})

# 链接库
target_link_libraries(doubao_analyzer 
    ${OpenCV_LIBS}
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

# 测试程序
add_executable(test_config test/test_config.cpp src/utils.cpp)
target_link_libraries(test_config ${OpenCV_LIBS})

# 安装目标
install(TARGETS doubao_analyzer DESTINATION bin)

# 安装后信息
install(CODE "
    message(STATUS \"\\n\")
    message(STATUS \"🎉 豆包媒体分析工具安装完成!\")
    message(STATUS \"💡 使用方法: doubao_analyzer --help\")
    message(STATUS \"🔧 交互模式: doubao_analyzer\")
    message(STATUS \"\\n\")
")
部署说明
一键部署
BASH
# 给脚本执行权限
chmod +x setup.sh install_deps.sh

# 运行部署脚本
./setup.sh
手动部署步骤
安装依赖:

BASH
sudo apt update
sudo apt install build-essential cmake pkg-config libopencv-dev libcurl4-openssl-dev nlohmann-json3-dev
编译项目:

BASH
mkdir build && cd build
cmake ..
make -j$(nproc)
安装到系统:

BASH
sudo make install
验证安装:

BASH
doubao_analyzer --help
测试安装
BASH
# 运行功能测试
cd build
./test_config

# 测试API连接 (需要有效API密钥)
doubao_analyzer --api-key YOUR_KEY --image test/test.jpg
这个C++17版本完全复现了Python版本的功能，包括：

✅ 图片和视频分析
✅ 批量处理
✅ 标签提取
✅ 交互式模式
✅ 结果保存
✅ 完整的错误处理
代码已针对Ubuntu系统优化，使用标准的C++17特性和现代CMake构建系统
=======
# doubao_analyzer
>>>>>>> origin/main
