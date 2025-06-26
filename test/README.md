# AT24C256 测试程序

这个目录包含用于测试AT24C256 EEPROM驱动程序的测试程序。

## 目录结构

```
test/
├── src/                    # 源代码目录
│   ├── camera_data_write.c # 相机参数写入程序
│   └── camera_data_read.c  # 相机参数读取程序
├── build/                  # 构建产物目录 (CMake生成)
│   ├── camera_data_write  # 可执行程序
│   ├── camera_data_read   # 可执行程序
│   └── CMake构建文件
├── camera_parameters/      # 测试数据文件目录
│   ├── camera0_intrinsics.dat
│   ├── camera0_rot_trans.dat
│   ├── camera1_intrinsics.dat
│   └── camera1_rot_trans.dat
├── out/                   # 读取程序输出目录（自动创建）
├── CMakeLists.txt        # CMake构建配置
└── README.md             # 本文档
```

## 测试程序说明

### camera_data_write - 写入程序

将 `camera_parameters` 目录中的所有 `.dat` 文件写入EEPROM。

#### 功能特性

- 自动遍历 `camera_parameters` 目录中的所有 `.dat` 文件
- 将文件写入EEPROM，从地址 `0x0000` 开始
- 支持最大32KB的文件大小
- 显示每个文件的写入进度和地址信息
- **只处理 `.dat` 文件**，忽略其他文件
- **可选擦除功能**：在写入前擦除整个EEPROM

#### 使用方法

```bash
# 构建测试程序
mkdir -p build && cd build
cmake -G Ninja .. && ninja

# 运行写入程序 (直接覆盖写入)
LD_LIBRARY_PATH=../build/lib ./camera_data_write

# 运行写入程序 (擦除后写入)
LD_LIBRARY_PATH=../build/lib ./camera_data_write --erase
```

#### 输出示例

```
相机参数EEPROM写入程序
======================
EEPROM设备初始化成功

=== 开始写入相机参数文件到EEPROM ===

处理文件: camera1_intrinsics.dat
写入文件到EEPROM: camera_parameters/camera1_intrinsics.dat (大小: 226 bytes, 地址: 0x0000)
✓ 文件写入成功: camera1_intrinsics.dat

...

=== 写入完成 ===
总共写入文件数: 4
EEPROM使用地址范围: 0x0000 - 0x02FF

✓ 写入成功！所有文件已成功写入EEPROM
```

### camera_data_read - 读取程序

从EEPROM读取相机参数数据并保存到 `out` 目录。

#### 功能特性

- 从EEPROM读取数据并保存到 `out` 目录
- 验证原始文件和读取文件的完整性
- 自动创建输出目录
- 显示每个文件的读取进度和验证结果
- **只处理 `.dat` 文件**，忽略其他文件

#### 使用方法

```bash
# 构建测试程序
mkdir -p build && cd build
cmake -G Ninja .. && ninja

# 运行读取程序
LD_LIBRARY_PATH=../build/lib ./camera_data_read
```

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

...

=== 读取完成 ===
总共读取文件数: 4
EEPROM使用地址范围: 0x0000 - 0x02FF

✓ 读取成功！所有文件已从EEPROM读取并保存
```

## 测试数据

测试程序使用以下相机参数文件（只处理 `.dat` 文件）：
- `camera0_intrinsics.dat` - 相机0内参文件 (222 bytes)
- `camera0_rot_trans.dat` - 相机0旋转平移文件 (60 bytes)
- `camera1_intrinsics.dat` - 相机1内参文件 (226 bytes)
- `camera1_rot_trans.dat` - 相机1旋转平移文件 (260 bytes)

## 完整使用流程

1. **构建主项目**:
   ```bash
   cd ..
   mkdir -p build && cd build
   cmake -G Ninja .. && ninja
   ```

2. **构建测试程序**:
   ```bash
   cd ../test
   mkdir -p build && cd build
   cmake -G Ninja .. && ninja
   ```

3. **写入数据到EEPROM**:
   ```bash
   # 直接覆盖写入 (推荐用于常规更新)
   LD_LIBRARY_PATH=../build/lib build/camera_data_write
   
   # 擦除后写入 (推荐用于首次使用或需要完全清理时)
   LD_LIBRARY_PATH=../build/lib build/camera_data_write --erase
   ```

4. **从EEPROM读取数据**:
   ```bash
   LD_LIBRARY_PATH=../build/lib build/camera_data_read
   ```

## 测试验证

两个程序都会自动验证以下内容：
- EEPROM设备初始化
- 文件操作（写入或读取）
- 文件内容完整性验证（读取程序）
- 输出目录创建（读取程序）
- **只处理 `.dat` 文件**，忽略其他文件

所有验证通过后，程序会显示相应的成功信息。

## 关于EEPROM擦除的说明

### 是否需要擦除？

**通常不需要全片擦除的情况：**
- **首次使用**：新EEPROM所有位都是1(0xFF)，可以直接写入
- **覆盖写入**：如果新数据完全覆盖旧数据区域，可以直接写入
- **小范围更新**：只修改部分数据时，不需要全片擦除

**建议使用擦除的情况：**
- **安全敏感应用**：防止残留数据泄露
- **数据完整性要求高**：确保没有旧数据干扰
- **从0改回1**：需要将已写入的0位改回1时

### 擦除性能考虑
- 全片擦除32KB EEPROM需要约2-3分钟
- 直接覆盖写入通常只需要几秒钟
- 在大多数应用场景中，直接覆盖写入是更高效的选择