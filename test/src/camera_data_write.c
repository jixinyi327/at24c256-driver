/**
 * @file camera_data_write.c
 * @brief 相机参数数据EEPROM写入程序
 *
 * 将camera_parameters目录中的文件写入EEPROM，并保存文件索引
 * 支持文件索引机制，确保读取程序可独立工作
 *
 * 使用说明：
 *   ./camera_data_write [--erase]
 *
 *   选项：
 *     --erase    在写入前擦除整个EEPROM (可选)
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
 * @brief 获取文件大小
 */
static long get_file_size(FILE* file) {
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
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
 * @brief 写入单个文件到EEPROM
 */
static int write_file_to_eeprom(at24c256_handle_t handle, const char* filename, 
                               uint16_t address, file_index_t* file_info) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("无法打开文件: %s\n", filename);
        return -1;
    }
    
    long file_size = get_file_size(file);
    if (file_size > MAX_FILE_SIZE) {
        printf("文件 %s 太大 (%ld bytes > %d bytes)\n", filename, file_size, MAX_FILE_SIZE);
        fclose(file);
        return -1;
    }
    
    // 读取文件内容
    uint8_t* buffer = (uint8_t*)malloc(file_size);
    if (!buffer) {
        printf("内存分配失败\n");
        fclose(file);
        return -1;
    }
    
    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        printf("读取文件 %s 失败\n", filename);
        free(buffer);
        return -1;
    }
    
    // 写入EEPROM
    printf("写入文件到EEPROM: %s (大小: %ld bytes, 地址: 0x%04X)\n", 
           filename, file_size, address);
    
    at24c256_err_t ret = at24c256_write(handle, address, buffer, file_size);
    if (ret != AT24C256_OK) {
        printf("EEPROM写入失败: %s\n", at24c256_strerror(ret));
        free(buffer);
        return -1;
    }
    
    // 等待写入完成
    sleep(1);
    
    // 填充文件索引信息
    strncpy(file_info->filename, strrchr(filename, '/') ? strrchr(filename, '/') + 1 : filename, 
            MAX_FILENAME_LENGTH - 1);
    file_info->filename[MAX_FILENAME_LENGTH - 1] = '\0';
    file_info->address = address;
    file_info->size = file_size;
    file_info->checksum = calculate_checksum(buffer, file_size);
    
    free(buffer);
    return 0;
}

/**
 * @brief 处理camera_parameters目录中的所有文件
 */
static int process_camera_parameters(at24c256_handle_t handle, const char* input_dir, 
                                   file_index_t* files, int* file_count) {
    DIR* dir = opendir(input_dir);
    if (!dir) {
        printf("无法打开目录: %s\n", input_dir);
        return -1;
    }
    
    struct dirent* entry;
    uint16_t current_address = EEPROM_START_ADDRESS + sizeof(index_header_t) + (MAX_FILES * sizeof(file_index_t));
    *file_count = 0;
    
    printf("\n=== 开始写入相机参数文件到EEPROM ===\n");
    
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
        snprintf(input_path, sizeof(input_path), "%s/%s", input_dir, entry->d_name);
        
        // 检查是否为常规文件
        struct stat path_stat;
        if (stat(input_path, &path_stat) == 0 && S_ISREG(path_stat.st_mode)) {
            printf("\n处理文件: %s\n", entry->d_name);
            
            // 写入文件到EEPROM
            if (write_file_to_eeprom(handle, input_path, current_address, &files[*file_count]) == 0) {
                (*file_count)++;
                current_address += files[*file_count - 1].size;
                printf("✓ 文件写入成功: %s (校验和: 0x%02X)\n", entry->d_name, files[*file_count - 1].checksum);
            } else {
                printf("✗ 文件写入失败: %s\n", entry->d_name);
            }
        }
    }
    
    closedir(dir);
    return *file_count;
}

/**
 * @brief 写入文件索引到EEPROM
 */
static int write_file_index(at24c256_handle_t handle, const file_index_t* files, int file_count) {
    index_header_t header;
    
    // 填充索引头
    memcpy(header.magic, "CAM\0", 4);
    header.version = 1;
    header.file_count = file_count;
    header.total_size = 0;
    memset(header.reserved, 0, sizeof(header.reserved));
    
    // 计算总大小
    for (int i = 0; i < file_count; i++) {
        header.total_size += files[i].size;
    }
    
    printf("\n=== 写入文件索引 ===\n");
    printf("文件数量: %d, 总大小: %d bytes\n", file_count, header.total_size);
    
    // 写入索引头
    at24c256_err_t ret = at24c256_write(handle, EEPROM_START_ADDRESS, (uint8_t*)&header, sizeof(header));
    if (ret != AT24C256_OK) {
        printf("写入索引头失败: %s\n", at24c256_strerror(ret));
        return -1;
    }
    
    // 写入文件索引
    uint16_t index_address = EEPROM_START_ADDRESS + sizeof(header);
    for (int i = 0; i < file_count; i++) {
        ret = at24c256_write(handle, index_address, (uint8_t*)&files[i], sizeof(file_index_t));
        if (ret != AT24C256_OK) {
            printf("写入文件索引 %d 失败: %s\n", i, at24c256_strerror(ret));
            return -1;
        }
        index_address += sizeof(file_index_t);
        
        printf("索引 %d: %s (地址: 0x%04X, 大小: %d, 校验和: 0x%02X)\n", 
               i, files[i].filename, files[i].address, files[i].size, files[i].checksum);
    }
    
    printf("✓ 文件索引写入完成\n");
    return 0;
}

/**
 * @brief 检查是否需要擦除EEPROM
 */
static bool should_erase_eeprom(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--erase") == 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 擦除整个EEPROM
 */
static at24c256_err_t erase_entire_eeprom(at24c256_handle_t handle) {
    printf("正在擦除整个EEPROM (32KB)...\n");
    
    // 分配擦除缓冲区
    uint8_t* erase_buffer = (uint8_t*)malloc(4096); // 4KB缓冲区
    if (!erase_buffer) {
        return AT24C256_ERROR_MEMORY;
    }
    
    memset(erase_buffer, 0xFF, 4096);
    
    // 分块擦除，避免一次性分配32KB内存
    uint16_t address = 0;
    uint16_t remaining = 32768;
    
    while (remaining > 0) {
        uint16_t chunk_size = (remaining > 4096) ? 4096 : remaining;
        
        at24c256_err_t ret = at24c256_write(handle, address, erase_buffer, chunk_size);
        if (ret != AT24C256_OK) {
            free(erase_buffer);
            return ret;
        }
        
        address += chunk_size;
        remaining -= chunk_size;
        
        // 显示进度
        int progress = (address * 100) / 32768;
        printf("\r擦除进度: %d%%", progress);
        fflush(stdout);
    }
    
    free(erase_buffer);
    printf("\n✓ EEPROM擦除完成\n");
    return AT24C256_OK;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    printf("相机参数EEPROM写入程序（修复版）\n");
    printf("==============================\n");
    
    bool erase_before_write = should_erase_eeprom(argc, argv);
    if (erase_before_write) {
        printf("模式: 擦除后写入\n");
    } else {
        printf("模式: 直接覆盖写入\n");
        printf("提示: 使用 --erase 参数可在写入前擦除整个EEPROM\n");
    }
    
    const char* input_dir = "camera_parameters";
    
    // 初始化EEPROM设备
    at24c256_config_t config = AT24C256_DEFAULT_CONFIG;
    at24c256_handle_t handle;
    
    at24c256_err_t ret = at24c256_init(&config, &handle);
    if (ret != AT24C256_OK) {
        printf("EEPROM初始化失败: %s\n", at24c256_strerror(ret));
        return EXIT_FAILURE;
    }
    
    printf("EEPROM设备初始化成功\n");
    
    // 如果需要，先擦除整个EEPROM
    if (erase_before_write) {
        ret = erase_entire_eeprom(handle);
        if (ret != AT24C256_OK) {
            printf("错误: EEPROM擦除失败 - %s\n", at24c256_strerror(ret));
            at24c256_deinit(handle);
            return EXIT_FAILURE;
        }
        printf("\n");
    }
    
    // 处理相机参数文件
    file_index_t files[MAX_FILES];
    int file_count = 0;
    int processed_count = process_camera_parameters(handle, input_dir, files, &file_count);
    
    if (processed_count > 0) {
        // 写入文件索引
        if (write_file_index(handle, files, file_count) != 0) {
            printf("错误: 文件索引写入失败\n");
            at24c256_deinit(handle);
            return EXIT_FAILURE;
        }
        
        printf("\n=== 写入完成 ===\n");
        printf("总共写入文件数: %d\n", file_count);
        printf("EEPROM使用地址范围: 0x%04X - 0x%04X\n", 
               EEPROM_START_ADDRESS, EEPROM_START_ADDRESS + sizeof(index_header_t) + 
               (file_count * sizeof(file_index_t)) + files[file_count - 1].size - 1);
    }
    
    // 清理资源
    at24c256_deinit(handle);
    
    if (processed_count > 0) {
        printf("\n✓ 写入成功！所有文件已成功写入EEPROM\n");
        return EXIT_SUCCESS;
    } else {
        printf("\n✗ 写入失败！没有成功写入任何文件\n");
        return EXIT_FAILURE;
    }
}