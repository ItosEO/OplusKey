# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.22.1)

# 声明项目名称
project("OplusKey")

# 启用 C++17 标准，因为我们使用了 <filesystem>
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(oplus_key main.cpp)
target_link_libraries(oplus_key log)
