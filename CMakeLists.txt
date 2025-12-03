cmake_minimum_required(VERSION 3.10)
project(doubao_analyzer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE "Debug")
# 注释掉或移除设置高优化等级的选项，例如：set(CMAKE_CXX_FLAGS "-O3")

# 查找依赖
find_package(OpenCV REQUIRED)
find_package(CUDA REQUIRED)  # 添加CUDA支持
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)

# 如果找到CUDA，定义HAVE_CUDA宏并添加包含路径
if(CUDA_FOUND)
    add_definitions(-DHAVE_CUDA)
    include_directories(${CUDA_INCLUDE_DIRS})
endif()

# 在第11行附近，将原来的 find_package(MySQL REQUIRED) 替换为：

# 方法1：使用pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)

set(MYSQL_INCLUDE_DIRS "/usr/include/mysql")
set(MYSQL_LIBRARIES "/usr/lib/x86_64-linux-gnu/libmysqlclient.so")


# 然后修改后续的包含目录和链接库
include_directories(${MYSQL_INCLUDE_DIRS})
# 在target_link_libraries中添加 ${MYSQL_LINK_LIBRARIES} 


pkg_check_modules(MYSQL REQUIRED mysqlclient)


# 包含目录
include_directories(include)

# 源文件
set(SOURCES
    src/main.cpp
    src/DoubaoMediaAnalyzer.cpp
    src/DoubaoMediaAnalyzer_db.cpp
    src/utils.cpp
    src/config.cpp
    src/DatabaseManager.cpp
    src/DatabaseManager_extended.cpp
    src/ConfigManager.cpp
    src/VideoKeyframeAnalyzer.cpp
    src/CurlConnectionPool.cpp
    src/TaskManager.cpp
    src/GPUManager.cpp
    src/Jwt.cpp
    src/RefreshTokenStore.cpp
    src/ExcelProcessor.cpp
)

# API服务器源文件
set(API_SOURCES
    src/api_main.cpp
    src/ApiServer.cpp
    src/DoubaoMediaAnalyzer.cpp
    src/DoubaoMediaAnalyzer_db.cpp
    src/utils.cpp
    src/config.cpp
    src/DatabaseManager.cpp
    src/DatabaseManager_extended.cpp
    src/ConfigManager.cpp
    src/VideoKeyframeAnalyzer.cpp
    src/CurlConnectionPool.cpp
    src/TaskManager.cpp
    src/GPUManager.cpp
    src/Jwt.cpp
    src/RefreshTokenStore.cpp
    src/ExcelProcessor.cpp
)

# 创建可执行文件
add_executable(doubao_analyzer ${SOURCES})
add_executable(doubao_api_server ${API_SOURCES})

# 链接库
target_link_libraries(doubao_analyzer
    ${OpenCV_LIBS}
    ${CUDA_LIBRARIES}
    CURL::libcurl
    ${MYSQL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

# API服务器链接库
target_link_libraries(doubao_api_server
    ${OpenCV_LIBS}
    ${CUDA_LIBRARIES}
    CURL::libcurl
    ${MYSQL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

# 设置C++标准
target_compile_features(doubao_analyzer PRIVATE cxx_std_17)
target_compile_features(doubao_api_server PRIVATE cxx_std_17)

# 安装目标
install(TARGETS doubao_analyzer doubao_api_server DESTINATION bin)
