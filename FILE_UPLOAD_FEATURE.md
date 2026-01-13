# 文件上传功能说明

## 功能概述

在 `alloc` 命令中添加了文件上传功能，支持上传 `.txt` 格式的文件到内存池。

## 使用方法

### 1. 基本语法

```bash
alloc "<description>" @<filepath>
```

或者

```bash
alloc "<description>" "<filepath>"
```

### 2. 使用示例

#### 示例 1：使用 @ 前缀上传文件
```bash
alloc "文档" @test_file.txt
```

#### 示例 2：直接使用文件路径（必须以 .txt 结尾）
```bash
alloc "文档" "test_file.txt"
```

#### 示例 3：使用完整路径
```bash
alloc "配置" @C:\Users\Documents\config.txt
```

### 3. 传统文本内容（仍然支持）

```bash
alloc "用户数据" "Hello World"
```

## 功能特性

1. **文件格式限制**：目前仅支持 `.txt` 文件
2. **文件大小检查**：自动检查文件大小，确保不超过内存池限制（预留 1MB 空间）
3. **错误处理**：
   - 文件不存在时显示错误信息
   - 文件格式不正确时显示错误信息
   - 文件过大时显示错误信息

## 实现细节

### 文件检测逻辑

系统会检测 content 参数：
- 如果以 `@` 开头，视为文件路径
- 如果以 `.txt` 结尾，视为文件路径
- 否则视为普通文本内容

### 文件读取流程

1. 检查文件扩展名是否为 `.txt`
2. 打开文件（二进制模式）
3. 获取文件大小
4. 检查文件大小是否超过限制
5. 读取文件内容到内存
6. 将内容存储到内存池

## 代码位置

- **命令定义**：`server/command/commands.cpp` 第 26-29 行
- **文件读取函数**：`server/command/commands.cpp` 第 130-160 行
- **文件路径检测**：`server/command/commands.cpp` 第 162-181 行
- **alloc 命令实现**：`server/command/commands.cpp` 第 478-530 行

## 测试示例

### 创建测试文件

创建 `test_file.txt`：
```
这是一个测试文件
用于测试文件上传功能
包含多行内容
测试中文字符支持
```

### 执行上传命令

```bash
alloc "测试文档" @test_file.txt
```

### 预期输出

```
Reading file: test_file.txt (XX bytes)
Allocation successful. Memory ID: memory_00001
Description: 测试文档
File uploaded successfully (XX bytes)
Content stored at block 0
```

### 验证上传

```bash
read memory_00001
```

应该显示文件内容。

## 注意事项

1. **文件路径**：可以使用相对路径或绝对路径
2. **文件编码**：建议使用 UTF-8 编码的文本文件
3. **文件大小**：受内存池大小限制（当前 100MB，预留 1MB）
4. **文件格式**：目前仅支持 `.txt` 文件，未来可扩展支持其他格式

## 未来扩展

- [ ] 支持更多文件格式（如 `.json`, `.xml`, `.csv` 等）
- [ ] 支持文件分块上传（大文件）
- [ ] 支持文件元数据（文件名、大小、修改时间等）
- [ ] 支持二进制文件上传
