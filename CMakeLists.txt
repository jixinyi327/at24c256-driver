# AT24C256 Driver - CMake Configuration
# 支持CMake + Ninja构建系统

cmake_minimum_required(VERSION 3.10)
project(at24c256_driver C)

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 设置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 编译器选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -DDEBUG")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -DNDEBUG")
endif()

# 警告选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

# 包含目录
include_directories(include)

# 创建静态库
add_library(at24c256_static STATIC
    src/at24c256.c
)

# 设置静态库属性
set_target_properties(at24c256_static PROPERTIES
    OUTPUT_NAME "at24c256"
    VERSION "1.0.0"
    SOVERSION "1"
)

# 创建动态库
add_library(at24c256_shared SHARED
    src/at24c256.c
)

# 设置动态库属性
set_target_properties(at24c256_shared PROPERTIES
    OUTPUT_NAME "at24c256"
    VERSION "1.0.0"
    SOVERSION "1"
)

# 创建示例程序
add_executable(at24c256_example
    examples/main.c
)

# 链接示例程序到静态库
target_link_libraries(at24c256_example at24c256_static)

# 安装配置
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation directory" FORCE)
endif()

# 安装头文件
install(
    DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# 安装库文件
install(
    TARGETS at24c256_static at24c256_shared
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 安装示例程序
install(
    TARGETS at24c256_example
    RUNTIME DESTINATION bin
)

# 创建测试目标
enable_testing()

# 添加测试
add_test(
    NAME at24c256_basic_test
    COMMAND at24c256_example
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# 包配置
set(CPACK_PACKAGE_NAME "at24c256-driver")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AT24C256 EEPROM Driver")
set(CPACK_PACKAGE_VENDOR "Driver Developer")
set(CPACK_PACKAGE_CONTACT "developer@example.com")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Developer <developer@example.com>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6")

include(CPack)

# 打印构建信息
message(STATUS "AT24C256 Driver Build Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "  Targets:")
message(STATUS "    - at24c256_static (static library)")
message(STATUS "    - at24c256_shared (shared library)")
message(STATUS "    - at24c256_example (example program)")