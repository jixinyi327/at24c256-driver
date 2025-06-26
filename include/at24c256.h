/**
 * @file at24c256.h
 * @brief AT24C256 EEPROM Driver
 * 
 * 一个易于移植的AT24C256 EEPROM驱动程序，支持跨页写入、错误处理和配置选项。
 * 
 * @author Driver Developer
 * @date 2025
 * @version 1.0
 */

#ifndef AT24C256_H
#define AT24C256_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AT24C256设备配置结构体
 */
typedef struct {
    const char* i2c_bus;      /**< I2C总线路径，如 "/dev/i2c-5" */
    uint8_t device_addr;      /**< 设备地址 (通常为0x50) */
    uint16_t page_size;       /**< 页大小 (AT24C256为64字节) */
    uint32_t total_size;      /**< 总容量 (AT24C256为32768字节) */
    uint16_t write_delay_ms;  /**< 写入延迟时间(毫秒) */
} at24c256_config_t;

/**
 * @brief AT24C256设备句柄
 */
typedef struct at24c256_dev_s* at24c256_handle_t;

/**
 * @brief 错误码定义
 */
typedef enum {
    AT24C256_OK = 0,              /**< 操作成功 */
    AT24C256_ERROR_INIT = -1,     /**< 初始化失败 */
    AT24C256_ERROR_WRITE = -2,    /**< 写入失败 */
    AT24C256_ERROR_READ = -3,     /**< 读取失败 */
    AT24C256_ERROR_PARAM = -4,    /**< 参数错误 */
    AT24C256_ERROR_MEMORY = -5,   /**< 内存分配失败 */
    AT24C256_ERROR_BUSY = -6,     /**< 设备忙 */
    AT24C256_ERROR_TIMEOUT = -7,  /**< 操作超时 */
} at24c256_err_t;

/**
 * @brief 默认配置
 */
#define AT24C256_DEFAULT_CONFIG { \
    .i2c_bus = "/dev/i2c-5",      \
    .device_addr = 0x50,          \
    .page_size = 64,              \
    .total_size = 32768,          \
    .write_delay_ms = 5           \
}

/**
 * @brief 初始化AT24C256设备
 * 
 * @param config 设备配置
 * @param handle 返回的设备句柄
 * @return at24c256_err_t 错误码
 */
at24c256_err_t at24c256_init(const at24c256_config_t* config, at24c256_handle_t* handle);

/**
 * @brief 释放AT24C256设备资源
 * 
 * @param handle 设备句柄
 * @return at24c256_err_t 错误码
 */
at24c256_err_t at24c256_deinit(at24c256_handle_t handle);

/**
 * @brief 从EEPROM读取数据
 * 
 * @param handle 设备句柄
 * @param address 起始地址 (0-32767)
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return at24c256_err_t 错误码
 */
at24c256_err_t at24c256_read(at24c256_handle_t handle, uint16_t address, 
                            uint8_t* data, uint16_t length);

/**
 * @brief 向EEPROM写入数据
 * 
 * @param handle 设备句柄
 * @param address 起始地址 (0-32767)
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return at24c256_err_t 错误码
 */
at24c256_err_t at24c256_write(at24c256_handle_t handle, uint16_t address, 
                             const uint8_t* data, uint16_t length);

/**
 * @brief 擦除EEPROM区域 (写入0xFF)
 * 
 * @param handle 设备句柄
 * @param address 起始地址
 * @param length 擦除长度
 * @return at24c256_err_t 错误码
 */
at24c256_err_t at24c256_erase(at24c256_handle_t handle, uint16_t address, uint16_t length);

/**
 * @brief 检查EEPROM是否就绪
 * 
 * @param handle 设备句柄
 * @param timeout_ms 超时时间(毫秒)
 * @return at24c256_err_t 错误码
 */
at24c256_err_t at24c256_wait_ready(at24c256_handle_t handle, uint32_t timeout_ms);

/**
 * @brief 获取设备信息
 * 
 * @param handle 设备句柄
 * @param config 返回的设备配置
 * @return at24c256_err_t 错误码
 */
at24c256_err_t at24c256_get_info(at24c256_handle_t handle, at24c256_config_t* config);

/**
 * @brief 获取错误描述
 * 
 * @param err 错误码
 * @return const char* 错误描述字符串
 */
const char* at24c256_strerror(at24c256_err_t err);

#ifdef __cplusplus
}
#endif

#endif /* AT24C256_H */