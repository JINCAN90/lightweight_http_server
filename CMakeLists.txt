# 设置CMake的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(LightweightHttpServer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)

# 添加头文件搜索路径
include_directories(include)

# 添加可执行文件
add_executable(${PROJECT_NAME} 
    src/EpollHandler.cpp
    src/ThreadPool.cpp
    # 这里需要添加你的主程序文件，例如 main.cpp，如果有的话
    # src/main.cpp
)

# 链接必要的库
target_link_libraries(${PROJECT_NAME}
    pthread  # 链接线程库
)