# AT24C256 EEPROM 驱动程序

一个易于移植的AT24C256 EEPROM驱动程序，支持跨页写入、错误处理和配置选项。

## 特性

- ✅ 易于移植的API设计
- ✅ 支持跨页写入操作
- ✅ 完整的错误处理机制
- ✅ 静态库和动态库支持
- ✅ CMake + Ninja 构建系统
- ✅ 详细的示例程序
- ✅ 完整的文档说明

## 硬件要求

- AT24C256 EEPROM芯片
- I2C总线 (本项目使用 `/dev/i2c-5`)
- Linux系统支持I2C设备

## 项目结构

```
at24c256_driver/
├── include/
│   └── at24c256.h          # 驱动程序头文件
├── src/
│   └── at24c256.c          # 驱动程序实现
├── examples/
│   └── main.c              # 示例程序
├── test/                   # 测试程序
│   ├── src/               # 测试程序源代码
│   │   ├── camera_data_write.c # 相机参数写入程序
│   │   └── camera_data_read.c  # 相机参数读取程序
│   ├── build/             # 测试程序构建产物
│   ├── camera_parameters/ # 测试数据文件
│   ├── CMakeLists.txt     # 测试程序CMake构建配置
│   └── README.md          # 测试程序说明
├── CMakeLists.txt          # CMake构建配置
└── README.md              # 本文档
```

## 快速开始

### 1. 构建项目

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目 (使用Ninja生成器)
cmake -G Ninja ..

# 构建项目
ninja
```

### 2. 运行示例程序

```bash
# 运行示例程序
./bin/at24c256_example
```

### 3. 运行测试程序

```bash
# 构建测试程序
cd test
mkdir -p build && cd build
cmake -G Ninja .. && ninja

# 运行写入程序（将相机参数写入EEPROM）
LD_LIBRARY_PATH=../../build/lib ./camera_data_write

# 运行写入程序（擦除后写入，可选）
LD_LIBRARY_PATH=../../build/lib ./camera_data_write --erase

# 运行读取程序（从EEPROM读取相机参数）
LD_LIBRARY_PATH=../../build/lib ./camera_data_read
```

### 4. 安装库文件 (可选)

```bash
# 安装到系统目录
sudo ninja install
```

## API 使用说明

### 初始化设备

```c
#include "at24c256.h"

// 使用默认配置
at24c256_config_t config = AT24C256_DEFAULT_CONFIG;
at24c256_handle_t handle;
at24c256_err_t ret = at24c256_init(&config, &handle);
if (ret != AT24C256_OK) {
    printf("初始化失败: %s\n", at24c256_strerror(ret));
    return -1;
}
```

### 自定义配置

```c
at24c256_config_t config = {
    .i2c_bus = "/dev/i2c-5",      // I2C总线路径
    .device_addr = 0x50,          // 设备地址
    .page_size = 64,              // 页大小
    .total_size = 32768,          // 总容量
    .write_delay_ms = 5           // 写入延迟
};
```

### 基本读写操作

```c
// 写入数据
const char* data = "Hello, EEPROM!";
ret = at24c256_write(handle, 0x1000, (uint8_t*)data, strlen(data) + 1);

// 读取数据
uint8_t buffer[64];
ret = at24c256_read(handle, 0x1000, buffer, strlen(data) + 1);
printf("读取的数据: %s\n", buffer);
```

### 擦除操作

```c
// 擦除32字节数据
ret = at24c256_erase(handle, 0x2000, 32);
```

### 等待设备就绪

```c
// 等待设备就绪，超时100ms
ret = at24c256_wait_ready(handle, 100);
```

### 清理资源

```c
// 释放设备资源
at24c256_deinit(handle);
```

## 错误处理

驱动程序提供完整的错误处理机制：

```c
at24c256_err_t ret = at24c256_write(handle, address, data, length);
if (ret != AT24C256_OK) {
    printf("操作失败: %s\n", at24c256_strerror(ret));
    // 处理错误
}
```

### 错误码说明

| 错误码 | 描述 |
|--------|------|
| `AT24C256_OK` | 操作成功 |
| `AT24C256_ERROR_INIT` | 初始化失败 |
| `AT24C256_ERROR_WRITE` | 写入失败 |
| `AT24C256_ERROR_READ` | 读取失败 |
| `AT24C256_ERROR_PARAM` | 参数错误 |
| `AT24C256_ERROR_MEMORY` | 内存分配失败 |
| `AT24C256_ERROR_BUSY` | 设备忙 |
| `AT24C256_ERROR_TIMEOUT` | 操作超时 |

## 构建选项

### 调试构建

```bash
mkdir build-debug
cd build-debug
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
ninja
```

### 发布构建

```bash
mkdir build-release
cd build-release
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

### 自定义安装路径

```bash
cmake -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

## 在你的项目中使用

### 1. 链接静态库

```cmake
# 在你的CMakeLists.txt中添加
target_link_libraries(your_target at24c256_static)
```

### 2. 链接动态库

```cmake
target_link_libraries(your_target at24c256_shared)
```

### 3. 包含头文件

```c
#include <at24c256.h>
```

## 示例程序输出

运行示例程序将执行以下测试：

1. **基础读写测试** - 验证基本读写功能
2. **跨页写入测试** - 测试跨页边界写入
3. **擦除测试** - 验证擦除功能
4. **性能测试** - 测量读写性能

成功输出示例：
```
AT24C256 驱动程序示例程序
==========================
设备配置:
  I2C总线: /dev/i2c-5
  设备地址: 0x50
  页大小: 64 字节
  总容量: 32768 字节
  写入延迟: 5 ms

设备初始化成功！

=== 基础读写测试 ===
写入数据到地址 0x1000: 'Hello, AT24C256 Driver! RK3588 Test.'
写入成功，等待写入完成...
从地址 0x1000 读取数据: 'Hello, AT24C256 Driver! RK3588 Test.'
✓ 数据验证成功！

=== 跨页写入测试 ===
写入 128 字节数据到地址 0x1FC0 (跨页边界)
跨页写入成功，等待写入完成...
✓ 跨页写入验证成功！

=== 擦除测试 ===
擦除地址 0x2000 的 32 字节数据
擦除成功，验证擦除结果...
✓ 擦除验证成功！所有字节均为0xFF

=== 性能测试 ===
写入 256 字节: 12.34 ms (20.75 KB/s)
读取 256 字节: 1.23 ms (208.13 KB/s)
数据验证: ✓ 通过

=== 测试结果 ===
总测试数: 4
通过测试: 4
失败测试: 0
✓ 所有测试通过！
```

## 测试程序

项目包含两个独立的相机参数存储测试程序，位于 `test/` 目录。

### camera_data_write - 写入程序

将 `camera_parameters` 目录中的所有 `.dat` 文件写入EEPROM。

#### 功能特性

- **自动文件处理**: 自动遍历 `camera_parameters` 目录中的所有 `.dat` 文件
- **EEPROM存储**: 将文件写入EEPROM并从地址 `0x0000` 开始存储
- **批量处理**: 支持批量处理多个文件，自动管理EEPROM地址空间
- **可选擦除**: 支持 `--erase` 参数在写入前擦除整个EEPROM
- **文件过滤**: 只处理 `.dat` 文件，忽略其他文件类型

#### 输出示例

```
相机参数EEPROM写入程序
======================
EEPROM设备初始化成功

=== 开始写入相机参数文件到EEPROM ===

处理文件: camera1_intrinsics.dat
写入文件到EEPROM: camera_parameters/camera1_intrinsics.dat (大小: 226 bytes, 地址: 0x0000)
✓ 文件写入成功: camera1_intrinsics.dat

=== 写入完成 ===
总共写入文件数: 5
EEPROM使用地址范围: 0x0000 - 0x1B03

✓ 写入成功！所有文件已成功写入EEPROM
```

### camera_data_read - 读取程序

从EEPROM读取相机参数数据并保存到 `out` 目录。

#### 功能特性

- **数据读取**: 从EEPROM读取数据并保存到输出目录
- **数据验证**: 验证原始文件和读取文件的完整性
- **自动目录创建**: 自动创建输出目录
- **文件过滤**: 只处理 `.dat` 文件，忽略其他文件类型

#### 输出示例

```
相机参数EEPROM读取程序
======================
创建目录: out
EEPROM设备初始化成功

=== 开始从EEPROM读取相机参数文件 ===

处理文件: camera1_intrinsics.dat
从EEPROM读取文件: out/camera1_intrinsics.dat (大小: 226 bytes, 地址: 0x0000)
成功保存文件: out/camera1_intrinsics.dat
✓ 文件验证成功: camera1_intrinsics.dat

=== 读取完成 ===
总共读取文件数: 5
EEPROM使用地址范围: 0x0000 - 0x1B03

✓ 读取成功！所有文件已从EEPROM读取并保存
```

### 测试数据

测试程序使用以下相机参数文件（只处理 `.dat` 文件）：
- `camera0_intrinsics.dat` - 相机0内参文件 (222 bytes)
- `camera0_rot_trans.dat` - 相机0旋转平移文件 (60 bytes)
- `camera1_intrinsics.dat` - 相机1内参文件 (226 bytes)
- `camera1_rot_trans.dat` - 相机1旋转平移文件 (260 bytes)

### 关于EEPROM擦除

#### 是否需要擦除？

**通常不需要全片擦除的情况：**
- **首次使用**: 新EEPROM所有位都是1(0xFF)，可以直接写入
- **覆盖写入**: 如果新数据完全覆盖旧数据区域，可以直接写入
- **小范围更新**: 只修改部分数据时，不需要全片擦除

**建议使用擦除的情况：**
- **安全敏感应用**: 防止残留数据泄露
- **数据完整性要求高**: 确保没有旧数据干扰
- **从0改回1**: 需要将已写入的0位改回1时

#### 擦除性能考虑
- 全片擦除32KB EEPROM需要约2-3分钟
- 直接覆盖写入通常只需要几秒钟
- 在大多数应用场景中，直接覆盖写入是更高效的选择

## 故障排除

### 1. 权限问题

确保对I2C设备有读写权限：
```bash
sudo chmod 666 /dev/i2c-5
```

或添加用户到i2c组：
```bash
sudo usermod -a -G i2c $USER
```

### 2. I2C设备不存在

检查I2C设备是否存在：
```bash
ls -la /dev/i2c-*
```

### 3. 构建错误

确保系统安装了必要的开发工具：
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build
```

## 许可证

本项目采用MIT许可证。详见LICENSE文件。

## 贡献

欢迎提交Issue和Pull Request来改进这个驱动程序。

## 联系方式

如有问题，请通过以下方式联系：
- 提交GitHub Issue
- 发送邮件至 developer@example.com