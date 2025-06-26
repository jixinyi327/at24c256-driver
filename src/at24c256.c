/**
 * @file at24c256.c
 * @brief AT24C256 EEPROM Driver Implementation
 * 
 * 实现AT24C256 EEPROM的读写操作，支持跨页写入和错误处理。
 */

#include "at24c256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <time.h>

/**
 * @brief AT24C256设备结构体
 */
struct at24c256_dev_s {
    int fd;                     /**< I2C文件描述符 */
    at24c256_config_t config;   /**< 设备配置 */
    bool initialized;           /**< 初始化标志 */
};

/**
 * @brief 错误描述字符串
 */
static const char* error_strings[] = {
    "Success",
    "Initialization failed",
    "Write operation failed",
    "Read operation failed",
    "Invalid parameter",
    "Memory allocation failed",
    "Device busy",
    "Operation timeout"
};

/**
 * @brief 检查地址和长度是否有效
 */
static at24c256_err_t check_address_length(at24c256_handle_t handle, 
                                          uint16_t address, uint16_t length) {
    if (!handle || !handle->initialized) {
        return AT24C256_ERROR_INIT;
    }
    
    if (address + length > handle->config.total_size) {
        return AT24C256_ERROR_PARAM;
    }
    
    if (length == 0) {
        return AT24C256_ERROR_PARAM;
    }
    
    return AT24C256_OK;
}

/**
 * @brief 等待设备就绪
 */
static at24c256_err_t internal_wait_ready(at24c256_handle_t handle, uint32_t timeout_ms) {
    struct timespec start, current;
    uint8_t dummy;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    while (1) {
        // 尝试读取一个字节来检查设备是否就绪
        if (read(handle->fd, &dummy, 1) >= 0) {
            return AT24C256_OK;
        }
        
        clock_gettime(CLOCK_MONOTONIC, &current);
        uint64_t elapsed_ms = (current.tv_sec - start.tv_sec) * 1000 + 
                             (current.tv_nsec - start.tv_nsec) / 1000000;
        
        if (elapsed_ms >= timeout_ms) {
            return AT24C256_ERROR_TIMEOUT;
        }
        
        usleep(1000); // 等待1ms
    }
}

at24c256_err_t at24c256_init(const at24c256_config_t* config, at24c256_handle_t* handle) {
    if (!config || !handle) {
        return AT24C256_ERROR_PARAM;
    }
    
    // 分配设备结构体
    at24c256_handle_t dev = (at24c256_handle_t)malloc(sizeof(struct at24c256_dev_s));
    if (!dev) {
        return AT24C256_ERROR_MEMORY;
    }
    
    memset(dev, 0, sizeof(struct at24c256_dev_s));
    
    // 复制配置
    memcpy(&dev->config, config, sizeof(at24c256_config_t));
    
    // 打开I2C总线
    dev->fd = open(config->i2c_bus, O_RDWR);
    if (dev->fd < 0) {
        free(dev);
        return AT24C256_ERROR_INIT;
    }
    
    // 设置I2C从设备地址
    if (ioctl(dev->fd, I2C_SLAVE, config->device_addr) < 0) {
        close(dev->fd);
        free(dev);
        return AT24C256_ERROR_INIT;
    }
    
    dev->initialized = true;
    *handle = dev;
    
    return AT24C256_OK;
}

at24c256_err_t at24c256_deinit(at24c256_handle_t handle) {
    if (!handle) {
        return AT24C256_ERROR_PARAM;
    }
    
    if (handle->fd >= 0) {
        close(handle->fd);
    }
    
    free(handle);
    return AT24C256_OK;
}

at24c256_err_t at24c256_read(at24c256_handle_t handle, uint16_t address, 
                            uint8_t* data, uint16_t length) {
    at24c256_err_t ret = check_address_length(handle, address, length);
    if (ret != AT24C256_OK) {
        return ret;
    }
    
    if (!data) {
        return AT24C256_ERROR_PARAM;
    }
    
    // 设置地址指针
    uint8_t addr_buffer[2] = {
        (uint8_t)((address >> 8) & 0xFF),
        (uint8_t)(address & 0xFF)
    };
    
    if (write(handle->fd, addr_buffer, 2) != 2) {
        return AT24C256_ERROR_READ;
    }
    
    // 读取数据
    ssize_t bytes_read = read(handle->fd, data, length);
    if (bytes_read != length) {
        return AT24C256_ERROR_READ;
    }
    
    return AT24C256_OK;
}

at24c256_err_t at24c256_write(at24c256_handle_t handle, uint16_t address, 
                             const uint8_t* data, uint16_t length) {
    at24c256_err_t ret = check_address_length(handle, address, length);
    if (ret != AT24C256_OK) {
        return ret;
    }
    
    if (!data) {
        return AT24C256_ERROR_PARAM;
    }
    
    uint16_t remaining = length;
    uint16_t current_addr = address;
    const uint8_t* current_data = data;
    
    while (remaining > 0) {
        // 计算当前页的剩余空间
        uint16_t page_offset = current_addr % handle->config.page_size;
        uint16_t bytes_in_page = handle->config.page_size - page_offset;
        
        // 限制写入字节数不超过剩余数据长度
        if (bytes_in_page > remaining) {
            bytes_in_page = remaining;
        }
        
        // 准备写入缓冲区 (地址 + 数据)
        uint8_t buffer[bytes_in_page + 2];
        buffer[0] = (uint8_t)((current_addr >> 8) & 0xFF);
        buffer[1] = (uint8_t)(current_addr & 0xFF);
        memcpy(&buffer[2], current_data, bytes_in_page);
        
        // 执行写入
        ssize_t bytes_written = write(handle->fd, buffer, bytes_in_page + 2);
        if (bytes_written != bytes_in_page + 2) {
            return AT24C256_ERROR_WRITE;
        }
        
        // 等待写入完成
        usleep(handle->config.write_delay_ms * 1000);
        
        // 更新指针和剩余长度
        current_addr += bytes_in_page;
        current_data += bytes_in_page;
        remaining -= bytes_in_page;
    }
    
    return AT24C256_OK;
}

at24c256_err_t at24c256_erase(at24c256_handle_t handle, uint16_t address, uint16_t length) {
    at24c256_err_t ret = check_address_length(handle, address, length);
    if (ret != AT24C256_OK) {
        return ret;
    }
    
    // 分配擦除缓冲区 (全部填充0xFF)
    uint8_t* erase_data = (uint8_t*)malloc(length);
    if (!erase_data) {
        return AT24C256_ERROR_MEMORY;
    }
    
    memset(erase_data, 0xFF, length);
    
    // 执行擦除写入
    ret = at24c256_write(handle, address, erase_data, length);
    
    free(erase_data);
    return ret;
}

at24c256_err_t at24c256_wait_ready(at24c256_handle_t handle, uint32_t timeout_ms) {
    if (!handle || !handle->initialized) {
        return AT24C256_ERROR_INIT;
    }
    
    return internal_wait_ready(handle, timeout_ms);
}

at24c256_err_t at24c256_get_info(at24c256_handle_t handle, at24c256_config_t* config) {
    if (!handle || !handle->initialized || !config) {
        return AT24C256_ERROR_PARAM;
    }
    
    memcpy(config, &handle->config, sizeof(at24c256_config_t));
    return AT24C256_OK;
}

const char* at24c256_strerror(at24c256_err_t err) {
    int index = -err;
    if (index < 0 || index >= (int)(sizeof(error_strings) / sizeof(error_strings[0]))) {
        return "Unknown error";
    }
    
    return error_strings[index];
}