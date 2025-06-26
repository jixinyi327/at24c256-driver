# AT24C256 测试程序 CMake配置

cmake_minimum_required(VERSION 3.10)
project(at24c256_test C)

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译器警告设置
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
endif()

# 包含目录
include_directories(../include)

# 查找AT24C256库
find_library(AT24C256_LIB at24c256 PATHS ../build/lib NO_DEFAULT_PATH)

if(NOT AT24C256_LIB)
    message(WARNING "AT24C256 library not found. Please build the main project first.")
endif()

# 写入程序
add_executable(camera_data_write src/camera_data_write.c)
target_link_libraries(camera_data_write ${AT24C256_LIB})

# 读取程序
add_executable(camera_data_read src/camera_data_read.c)
target_link_libraries(camera_data_read ${AT24C256_LIB})

# 安装目标（可选）
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation directory" FORCE)
endif()

install(TARGETS camera_data_write camera_data_read
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# 创建帮助信息
message(STATUS "AT24C256 Test Programs Build Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "  Targets:")
message(STATUS "    - camera_data_write (write camera parameters to EEPROM)")
message(STATUS "    - camera_data_read (read camera parameters from EEPROM)")