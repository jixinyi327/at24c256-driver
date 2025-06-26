/**
 * @file camera_data_read.c
 * @brief 相机参数数据EEPROM读取程序
 * 
 * 从EEPROM读取相机参数数据并保存到输出目录
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
 * @brief 从EEPROM读取文件并保存
 */
static int read_file_from_eeprom(at24c256_handle_t handle, const char* output_filename,
                                uint16_t address, long file_size) {
    // 从EEPROM读取数据
    uint8_t* buffer = (uint8_t*)malloc(file_size);
    if (!buffer) {
        printf("内存分配失败\n");
        return -1;
    }
    
    printf("从EEPROM读取文件: %s (大小: %ld bytes, 地址: 0x%04X)\n", 
           output_filename, file_size, address);
    
    at24c256_err_t ret = at24c256_read(handle, address, buffer, file_size);
    if (ret != AT24C256_OK) {
        printf("EEPROM读取失败: %s\n", at24c256_strerror(ret));
        free(buffer);
        return -1;
    }
    
    // 写入输出文件
    FILE* file = fopen(output_filename, "wb");
    if (!file) {
        printf("无法创建输出文件: %s\n", output_filename);
        free(buffer);
        return -1;
    }
    
    size_t bytes_written = fwrite(buffer, 1, file_size, file);
    fclose(file);
    free(buffer);
    
    if (bytes_written != (size_t)file_size) {
        printf("写入输出文件失败: %s\n", output_filename);
        return -1;
    }
    
    printf("成功保存文件: %s\n", output_filename);
    return 0;
}

/**
 * @brief 验证两个文件内容是否相同
 */
static int verify_files(const char* file1, const char* file2) {
    FILE* f1 = fopen(file1, "rb");
    FILE* f2 = fopen(file2, "rb");
    
    if (!f1 || !f2) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return -1;
    }
    
    int result = 0;
    uint8_t buf1[1024], buf2[1024];
    size_t bytes1, bytes2;
    
    do {
        bytes1 = fread(buf1, 1, sizeof(buf1), f1);
        bytes2 = fread(buf2, 1, sizeof(buf2), f2);
        
        if (bytes1 != bytes2 || memcmp(buf1, buf2, bytes1) != 0) {
            result = -1;
            break;
        }
    } while (bytes1 > 0);
    
    fclose(f1);
    fclose(f2);
    return result;
}

/**
 * @brief 处理camera_parameters目录中的所有文件
 */
static int process_camera_parameters(at24c256_handle_t handle, const char* input_dir, 
                                   const char* output_dir) {
    DIR* dir = opendir(input_dir);
    if (!dir) {
        printf("无法打开目录: %s\n", input_dir);
        return -1;
    }
    
    struct dirent* entry;
    uint16_t current_address = EEPROM_START_ADDRESS;
    int file_count = 0;
    
    printf("\n=== 开始从EEPROM读取相机参数文件 ===\n");
    
    // 遍历目录中的所有文件
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 .. 目录
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 只处理 .dat 文件
        char* extension = strrchr(entry->d_name, '.');
        if (extension == NULL || strcmp(extension, ".dat") != 0) {
            continue;
        }
        
        char input_path[512];
        char output_path[512];
        
        snprintf(input_path, sizeof(input_path), "%s/%s", input_dir, entry->d_name);
        snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, entry->d_name);
        
        // 检查是否为常规文件
        struct stat path_stat;
        if (stat(input_path, &path_stat) == 0 && S_ISREG(path_stat.st_mode)) {
            printf("\n处理文件: %s\n", entry->d_name);
            
            // 获取文件大小
            FILE* file = fopen(input_path, "rb");
            if (!file) {
                printf("无法打开文件: %s\n", input_path);
                continue;
            }
            
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fclose(file);
            
            // 从EEPROM读取并保存到输出目录
            if (read_file_from_eeprom(handle, output_path, current_address, file_size) == 0) {
                file_count++;
                current_address += file_size;
                
                // 验证文件内容
                if (verify_files(input_path, output_path) == 0) {
                    printf("✓ 文件验证成功: %s\n", entry->d_name);
                } else {
                    printf("✗ 文件验证失败: %s\n", entry->d_name);
                }
            }
        }
    }
    
    closedir(dir);
    printf("\n=== 读取完成 ===\n");
    printf("总共读取文件数: %d\n", file_count);
    printf("EEPROM使用地址范围: 0x%04X - 0x%04X\n", 
           EEPROM_START_ADDRESS, current_address - 1);
    
    return file_count;
}

/**
 * @brief 主函数
 */
int main(void) {
    printf("相机参数EEPROM读取程序\n");
    printf("======================\n");
    
    const char* input_dir = "camera_parameters";
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
    
    // 处理相机参数文件
    int file_count = process_camera_parameters(handle, input_dir, output_dir);
    
    // 清理资源
    at24c256_deinit(handle);
    
    if (file_count > 0) {
        printf("\n✓ 读取成功！所有文件已从EEPROM读取并保存\n");
        return EXIT_SUCCESS;
    } else {
        printf("\n✗ 读取失败！没有成功读取任何文件\n");
        return EXIT_FAILURE;
    }
}