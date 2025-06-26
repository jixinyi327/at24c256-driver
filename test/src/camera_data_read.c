/**
 * @file camera_data_read.c
 * @brief 相机参数数据EEPROM读取程序
 *
 * 从EEPROM读取相机参数数据并保存到输出目录
 * 支持文件索引机制，不依赖本地目录
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include "at24c256.h"

#define EEPROM_START_ADDRESS 0x0000
#define MAX_FILE_SIZE (32 * 1024)  // 最大文件大小32KB
#define MAX_FILENAME_LENGTH 64
#define MAX_FILES 16

/**
 * @brief 文件索引结构
 */
typedef struct {
    char filename[MAX_FILENAME_LENGTH];
    uint16_t address;
    uint16_t size;
    uint8_t checksum;
} file_index_t;

/**
 * @brief 文件索引头
 */
typedef struct {
    uint8_t magic[4];  // "CAM\0"
    uint8_t version;
    uint8_t file_count;
    uint16_t total_size;
    uint8_t reserved[8];
} index_header_t;

/**
 * @brief 创建目录（如果不存在）
 */
static int create_directory(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) == -1) {
            perror("创建目录失败");
            return -1;
        }
        printf("创建目录: %s\n", path);
    }
    return 0;
}

/**
 * @brief 计算数据的校验和
 */
static uint8_t calculate_checksum(const uint8_t* data, size_t size) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief 从EEPROM读取文件索引
 */
static int read_file_index(at24c256_handle_t handle, file_index_t* files, int* file_count) {
    index_header_t header;
    
    // 读取索引头
    at24c256_err_t ret = at24c256_read(handle, EEPROM_START_ADDRESS, (uint8_t*)&header, sizeof(header));
    if (ret != AT24C256_OK) {
        printf("读取索引头失败: %s\n", at24c256_strerror(ret));
        return -1;
    }
    
    // 验证魔术字
    if (memcmp(header.magic, "CAM\0", 4) != 0) {
        printf("无效的索引格式 (魔术字不匹配)\n");
        return -1;
    }
    
    printf("索引版本: %d, 文件数量: %d\n", header.version, header.file_count);
    
    if (header.file_count > MAX_FILES) {
        printf("文件数量超出限制: %d > %d\n", header.file_count, MAX_FILES);
        return -1;
    }
    
    // 读取文件索引
    uint16_t index_address = EEPROM_START_ADDRESS + sizeof(header);
    for (int i = 0; i < header.file_count; i++) {
        ret = at24c256_read(handle, index_address, (uint8_t*)&files[i], sizeof(file_index_t));
        if (ret != AT24C256_OK) {
            printf("读取文件索引 %d 失败: %s\n", i, at24c256_strerror(ret));
            return -1;
        }
        index_address += sizeof(file_index_t);
    }
    
    *file_count = header.file_count;
    return 0;
}

/**
 * @brief 从EEPROM读取文件并保存
 */
static int read_file_from_eeprom(at24c256_handle_t handle, const file_index_t* file_info,
                                const char* output_dir) {
    char output_path[512];
    snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, file_info->filename);
    
    // 从EEPROM读取数据
    uint8_t* buffer = (uint8_t*)malloc(file_info->size);
    if (!buffer) {
        printf("内存分配失败\n");
        return -1;
    }
    
    printf("从EEPROM读取文件: %s (大小: %d bytes, 地址: 0x%04X)\n", 
           file_info->filename, file_info->size, file_info->address);
    
    at24c256_err_t ret = at24c256_read(handle, file_info->address, buffer, file_info->size);
    if (ret != AT24C256_OK) {
        printf("EEPROM读取失败: %s\n", at24c256_strerror(ret));
        free(buffer);
        return -1;
    }
    
    // 验证校验和
    uint8_t checksum = calculate_checksum(buffer, file_info->size);
    if (checksum != file_info->checksum) {
        printf("校验和验证失败: 期望 0x%02X, 实际 0x%02X\n", file_info->checksum, checksum);
        free(buffer);
        return -1;
    }
    
    // 写入输出文件
    FILE* file = fopen(output_path, "wb");
    if (!file) {
        printf("无法创建输出文件: %s\n", output_path);
        free(buffer);
        return -1;
    }
    
    size_t bytes_written = fwrite(buffer, 1, file_info->size, file);
    fclose(file);
    free(buffer);
    
    if (bytes_written != (size_t)file_info->size) {
        printf("写入输出文件失败: %s\n", output_path);
        return -1;
    }
    
    printf("✓ 成功保存文件: %s (校验和: 0x%02X)\n", output_path, checksum);
    return 0;
}

/**
 * @brief 主函数
 */
int main(void) {
    printf("相机参数EEPROM读取程序（修复版）\n");
    printf("==============================\n");
    
    const char* output_dir = "out";
    
    // 创建输出目录
    if (create_directory(output_dir) != 0) {
        return EXIT_FAILURE;
    }
    
    // 初始化EEPROM设备
    at24c256_config_t config = AT24C256_DEFAULT_CONFIG;
    at24c256_handle_t handle;
    
    at24c256_err_t ret = at24c256_init(&config, &handle);
    if (ret != AT24C256_OK) {
        printf("EEPROM初始化失败: %s\n", at24c256_strerror(ret));
        return EXIT_FAILURE;
    }
    
    printf("EEPROM设备初始化成功\n");
    
    // 读取文件索引
    file_index_t files[MAX_FILES];
    int file_count = 0;
    
    printf("\n=== 读取文件索引 ===\n");
    if (read_file_index(handle, files, &file_count) != 0) {
        printf("读取文件索引失败，可能EEPROM中没有有效数据\n");
        at24c256_deinit(handle);
        return EXIT_FAILURE;
    }
    
    printf("成功读取 %d 个文件的索引\n", file_count);
    
    // 读取文件数据
    printf("\n=== 开始从EEPROM读取相机参数文件 ===\n");
    int success_count = 0;
    
    for (int i = 0; i < file_count; i++) {
        if (read_file_from_eeprom(handle, &files[i], output_dir) == 0) {
            success_count++;
        }
    }
    
    // 清理资源
    at24c256_deinit(handle);
    
    if (success_count > 0) {
        printf("\n✓ 读取成功！%d/%d 个文件已从EEPROM读取并保存\n", success_count, file_count);
        return EXIT_SUCCESS;
    } else {
        printf("\n✗ 读取失败！没有成功读取任何文件\n");
        return EXIT_FAILURE;
    }
}