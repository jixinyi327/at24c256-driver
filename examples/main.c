/**
 * @file main.c
 * @brief AT24C256驱动程序示例程序
 *
 * 演示如何使用AT24C256驱动程序进行基本的读写操作。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "at24c256.h"

/**
 * @brief 打印十六进制数据
 */
static void print_hex(const uint8_t* data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    if (length % 16 != 0) {
        printf("\n");
    }
}

/**
 * @brief 基础读写测试
 */
static int basic_read_write_test(at24c256_handle_t handle) {
    printf("\n=== 基础读写测试 ===\n");
    
    const char* test_string = "Hello, AT24C256 Driver! RK3588 Test.";
    uint16_t test_address = 0x1000;
    uint16_t data_length = strlen(test_string) + 1;
    
    printf("写入数据到地址 0x%04X: '%s'\n", test_address, test_string);
    
    // 写入数据
    at24c256_err_t ret = at24c256_write(handle, test_address, 
                                       (const uint8_t*)test_string, data_length);
    if (ret != AT24C256_OK) {
        printf("写入失败: %s\n", at24c256_strerror(ret));
        return -1;
    }
    
    printf("写入成功，等待写入完成...\n");
    usleep(10000); // 等待10ms
    
    // 读取数据
    uint8_t read_buffer[64] = {0};
    ret = at24c256_read(handle, test_address, read_buffer, data_length);
    if (ret != AT24C256_OK) {
        printf("读取失败: %s\n", at24c256_strerror(ret));
        return -1;
    }
    
    printf("从地址 0x%04X 读取数据: '%s'\n", test_address, read_buffer);
    
    // 验证数据
    if (memcmp(test_string, read_buffer, data_length) == 0) {
        printf("✓ 数据验证成功！\n");
        return 0;
    } else {
        printf("✗ 数据验证失败！\n");
        return -1;
    }
}

/**
 * @brief 跨页写入测试
 */
static int cross_page_write_test(at24c256_handle_t handle) {
    printf("\n=== 跨页写入测试 ===\n");
    
    uint16_t page_boundary = 0x1FC0; // 接近页边界
    uint16_t test_length = 128;      // 跨越多个页
    
    // 准备测试数据
    uint8_t write_data[test_length];
    for (uint16_t i = 0; i < test_length; i++) {
        write_data[i] = 'A' + (i % 26);
    }
    
    printf("写入 %d 字节数据到地址 0x%04X (跨页边界)\n", test_length, page_boundary);
    
    // 写入跨页数据
    at24c256_err_t ret = at24c256_write(handle, page_boundary, write_data, test_length);
    if (ret != AT24C256_OK) {
        printf("跨页写入失败: %s\n", at24c256_strerror(ret));
        return -1;
    }
    
    printf("跨页写入成功，等待写入完成...\n");
    usleep(20000); // 等待20ms
    
    // 读取验证
    uint8_t read_data[test_length];
    ret = at24c256_read(handle, page_boundary, read_data, test_length);
    if (ret != AT24C256_OK) {
        printf("跨页读取失败: %s\n", at24c256_strerror(ret));
        return -1;
    }
    
    // 验证数据
    if (memcmp(write_data, read_data, test_length) == 0) {
        printf("✓ 跨页写入验证成功！\n");
        return 0;
    } else {
        printf("✗ 跨页写入验证失败！\n");
        return -1;
    }
}

/**
 * @brief 擦除测试
 */
static int erase_test(at24c256_handle_t handle) {
    printf("\n=== 擦除测试 ===\n");
    
    uint16_t erase_address = 0x2000;
    uint16_t erase_length = 32;
    
    printf("擦除地址 0x%04X 的 %d 字节数据\n", erase_address, erase_length);
    
    // 执行擦除
    at24c256_err_t ret = at24c256_erase(handle, erase_address, erase_length);
    if (ret != AT24C256_OK) {
        printf("擦除失败: %s\n", at24c256_strerror(ret));
        return -1;
    }
    
    printf("擦除成功，验证擦除结果...\n");
    usleep(10000);
    
    // 读取验证
    uint8_t read_data[erase_length];
    ret = at24c256_read(handle, erase_address, read_data, erase_length);
    if (ret != AT24C256_OK) {
        printf("读取验证失败: %s\n", at24c256_strerror(ret));
        return -1;
    }
    
    // 检查是否全部为0xFF
    int all_ff = 1;
    for (uint16_t i = 0; i < erase_length; i++) {
        if (read_data[i] != 0xFF) {
            all_ff = 0;
            break;
        }
    }
    
    if (all_ff) {
        printf("✓ 擦除验证成功！所有字节均为0xFF\n");
        return 0;
    } else {
        printf("✗ 擦除验证失败！数据不是全0xFF\n");
        printf("读取的数据: ");
        print_hex(read_data, erase_length);
        return -1;
    }
}

/**
 * @brief 性能测试
 */
static int performance_test(at24c256_handle_t handle) {
    printf("\n=== 性能测试 ===\n");
    
    uint16_t test_address = 0x3000;
    uint16_t test_size = 256;
    
    // 准备测试数据
    uint8_t test_data[test_size];
    for (uint16_t i = 0; i < test_size; i++) {
        test_data[i] = i % 256;
    }
    
    struct timespec start, end;
    
    // 写入性能测试
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    at24c256_err_t ret = at24c256_write(handle, test_address, test_data, test_size);
    if (ret != AT24C256_OK) {
        printf("性能测试写入失败: %s\n", at24c256_strerror(ret));
        return -1;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double write_time = (end.tv_sec - start.tv_sec) * 1000.0 + 
                       (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    // 等待写入完成
    usleep(50000);
    
    // 读取性能测试
    uint8_t read_back[test_size];
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    ret = at24c256_read(handle, test_address, read_back, test_size);
    if (ret != AT24C256_OK) {
        printf("性能测试读取失败: %s\n", at24c256_strerror(ret));
        return -1;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double read_time = (end.tv_sec - start.tv_sec) * 1000.0 + 
                      (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    // 验证数据
    int verified = memcmp(test_data, read_back, test_size) == 0;
    
    printf("写入 %d 字节: %.2f ms (%.2f KB/s)\n", 
           test_size, write_time, test_size / write_time);
    printf("读取 %d 字节: %.2f ms (%.2f KB/s)\n", 
           test_size, read_time, test_size / read_time);
    printf("数据验证: %s\n", verified ? "✓ 通过" : "✗ 失败");
    
    return verified ? 0 : -1;
}

/**
 * @brief 主函数
 */
int main(void) {
    printf("AT24C256 驱动程序示例程序\n");
    printf("==========================\n");
    
    // 使用默认配置
    at24c256_config_t config = AT24C256_DEFAULT_CONFIG;
    
    printf("设备配置:\n");
    printf("  I2C总线: %s\n", config.i2c_bus);
    printf("  设备地址: 0x%02X\n", config.device_addr);
    printf("  页大小: %d 字节\n", config.page_size);
    printf("  总容量: %d 字节\n", config.total_size);
    printf("  写入延迟: %d ms\n", config.write_delay_ms);
    
    // 初始化设备
    at24c256_handle_t handle;
    at24c256_err_t ret = at24c256_init(&config, &handle);
    if (ret != AT24C256_OK) {
        printf("设备初始化失败: %s\n", at24c256_strerror(ret));
        return EXIT_FAILURE;
    }
    
    printf("\n设备初始化成功！\n");
    
    int success_count = 0;
    int total_tests = 4;
    
    // 执行测试
    if (basic_read_write_test(handle) == 0) success_count++;
    if (cross_page_write_test(handle) == 0) success_count++;
    if (erase_test(handle) == 0) success_count++;
    if (performance_test(handle) == 0) success_count++;
    
    // 清理资源
    at24c256_deinit(handle);
    
    printf("\n=== 测试结果 ===\n");
    printf("总测试数: %d\n", total_tests);
    printf("通过测试: %d\n", success_count);
    printf("失败测试: %d\n", total_tests - success_count);
    
    if (success_count == total_tests) {
        printf("✓ 所有测试通过！\n");
        return EXIT_SUCCESS;
    } else {
        printf("✗ 部分测试失败！\n");
        return EXIT_FAILURE;
    }
}