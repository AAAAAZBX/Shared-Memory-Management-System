# Shared Memory Management System

## 项目简介

本项目是一个基于 C++17 实现的共享内存管理系统，采用固定块大小（4KB）的内存池管理策略，提供高效的内存分配、释放和紧凑功能。目前已完成服务器端核心功能的开发与测试。

## 核心特性

### 内存管理
- **固定块大小管理**：将内存池划分为 256 个 4KB 的固定大小块（当前配置为 1MB 内存池）
- **连续块分配**：支持跨多个块的数据存储，自动计算所需块数
- **内存紧凑（Compaction）**：当找不到连续空闲块但总空间足够时，自动进行内存紧凑，合并碎片
- **元数据管理**：每个块维护使用状态、用户标识和内容标识

### 已实现功能

#### 1. 内存池初始化与重置
- 自动分配 1MB 连续内存空间
- 支持内存池重置，清空所有分配信息

#### 2. 内存分配（`alloc`）
- 支持按用户和内容标识分配内存
- 自动计算所需块数（向上取整）
- 空间不足时自动触发紧凑操作
- 返回分配的起始块 ID

#### 3. 内存释放（`free` / `delete`）
- `free <user>`：释放指定用户的所有内存块
- `delete <user>`：释放指定用户的所有内存块（`free` 的别名）

#### 4. 内容更新（`update`）
- `update <user> "<new_content>"`：更新指定用户的内存内容
- 如果新内容大小超过原分配，自动重新分配
- 如果新内容大小不超过原分配，直接覆盖写入

#### 5. 内存紧凑（`compact`）
- 自动合并碎片，将已使用的块移动到内存池前端
- 更新用户块信息映射关系

#### 6. 状态查询（`status`）
- `status --memory`：显示内存池使用情况，按用户展示占用范围（格式：`block_000 - block_015`）
- `status --block`：显示所有 256 个块的状态，包括占用客户端信息（格式：`block_000`, `block_001` 等）

#### 7. 内容读取（`read`）
- `read <user>`：显示指定用户写入的内容
- 直接从内存池读取，遇到 0 停止
- 显示块范围、内容和大小

#### 8. 命令行界面
- 交互式 REPL（Read-Eval-Print Loop）界面
- 完整的帮助系统（`help` 命令）
- 优雅的退出机制（`quit` / `exit`）

#### 9. 数据持久化
- 自动保存内存池状态到 `memory_pool.dat` 文件
- 支持程序正常退出（`quit`/`exit`）时保存
- 支持异常退出（Ctrl+C、Ctrl+Z）时自动保存
- 程序启动时自动加载之前保存的状态
- 二进制文件格式，包含文件头、元数据、使用状态位图、用户块信息和内存池数据

#### 10. 批量执行（`exec`）
- `exec <filename>`：从文件读取并执行命令
- 支持空行和注释（以 `#` 开头）
- 显示执行的行号和命令
- 遇到 `quit`/`exit` 时停止执行（不退出程序）

#### 11. 内存池重置（`reset`）
- `reset`：清空所有内存数据并将元数据重置为默认状态
- 需要密码确认，防止误操作
- 重置后内存池恢复到初始状态（所有块为空闲）

## 技术架构

### 核心类设计

#### `SharedMemoryPool`
内存池管理核心类，负责：
- 内存块的分配与释放
- 元数据维护（`BlockMeta`）
- 空闲块追踪（`bitset`）
- 用户分配信息映射（`map<string, pair<size_t, size_t>>`）

#### 关键数据结构
```cpp
struct BlockMeta {
    bool used;           // 是否被使用
    std::string user;    // 用户标识（IP:端口号）
    std::string content; // 内容标识
};
```

### 内存布局
```
┌─────────────────────────────────────────┐
│  Memory Pool (1MB = 256 × 4KB blocks)  │
├─────────────────────────────────────────┤
│ Block 0   │ Block 1   │ ... │ Block 255 │
│ (4KB)     │ (4KB)     │     │ (4KB)     │
└─────────────────────────────────────────┘
```

### 数据持久化文件格式

程序退出时会自动将内存池状态保存到 `memory_pool.dat` 文件中，启动时会自动加载。文件格式如下：

#### 文件结构（按顺序）

```
┌─────────────────────────────────────────────────────────┐
│ 1. FileHeader (文件头)                                    │
│    - magic: uint32_t (0x4D454D50, "MEMP")                │
│    - version: uint32_t (当前版本: 1)                      │
│    - free_block_count: size_t (空闲块数量)                │
│    - user_info_count: size_t (用户信息条目数)             │
│    - reserved: uint64_t[4] (预留字段)                    │
├─────────────────────────────────────────────────────────┤
│ 2. BlockMeta Array (元数据数组，256个块)                  │
│    对每个块 i (0-255):                                    │
│    - used: bool (是否被使用)                              │
│    - userLen: size_t (用户标识长度)                      │
│    - user: char[userLen] (用户标识字符串，如果 userLen>0) │
│    - contentLen: size_t (内容标识长度)                   │
│    - content: char[contentLen] (内容标识，如果 contentLen>0)│
├─────────────────────────────────────────────────────────┤
│ 3. Used Map (使用状态位图)                                │
│    - bitsetBytes: uint8_t[(256+7)/8] (32字节)            │
│      (每个位表示对应块的使用状态)                          │
├─────────────────────────────────────────────────────────┤
│ 4. User Block Info (用户块信息映射)                       │
│    对每个用户条目:                                        │
│    - keyLen: size_t (用户标识长度)                       │
│    - key: char[keyLen] (用户标识字符串)                  │
│    - startBlock: size_t (起始块ID)                       │
│    - blockCount: size_t (块数量)                         │
├─────────────────────────────────────────────────────────┤
│ 5. Pool Data (内存池数据)                                 │
│    - pool_: uint8_t[1048576] (1MB 原始数据)              │
└─────────────────────────────────────────────────────────┘
```

#### 文件格式说明

- **文件类型**：二进制文件
- **字节序**：小端序（Little-Endian，Windows 默认）
- **魔数**：`0x4D454D50`（ASCII: "MEMP"），用于文件格式验证
- **版本号**：当前为 `1`，用于未来格式升级兼容

#### 文件大小估算

- 文件头：`sizeof(FileHeader)` ≈ 48 字节
- 元数据数组：256 × (1 + 8 + 可变字符串长度) ≈ 2-10 KB（取决于字符串长度）
- Used Map：32 字节
- User Block Info：可变（取决于用户数量）
- Pool Data：1,048,576 字节（1MB）
- **总大小**：约 1.05-1.06 MB

## 使用方法

### 编译与运行

#### 方式一：使用批处理脚本（推荐）
```bash
cd server
.\run.bat
```

#### 方式二：手动编译
```bash
cd server
g++ -std=c++17 -Wall main.cpp command/commands.cpp shared_memory_pool/shared_memory_pool.cpp persistence/persistence.cpp -o main.exe
.\main.exe
```

### 命令示例

```bash
# 查看帮助
server> help
server> help status

# 查看内存状态
server> status --memory
server> status --block

# 分配内存
server> alloc client_1 "Hello World"

# 读取用户内容
server> read client_1

# 释放用户内存
server> free client_1
server> delete client_1  # free 的别名

# 更新用户内容
server> update client_1 "Updated Content"

# 批量执行命令文件
server> exec sample.txt
[1] alloc 192.168.134.233:32415 "1145141919810"
Allocation successful. Content stored at block 0
[2] alloc 192.168.134.255:54321 "114514"
Allocation successful. Content stored at block 1
Finished executing commands from 'sample.txt'

# 紧凑内存
server> compact

# 重置内存池（需要密码确认）
server> reset

# 退出
server> quit
```

### 状态输出示例

**`status --memory` 输出：**
```
Memory Pool Status:
| range                 | Occupied Client     |
| --------------------- | ------------------- |
| block_000 - block_015 | 192.168.1.100:54321 |
| block_016 - block_031 | client_2            |
```

**`status --block` 输出：**
```
Block Pool Status:
| ID        | Occupied Client     |
| --------- | ------------------- |
| block_000 | 192.168.1.100:54321 |
| block_001 | 192.168.1.100:54321 |
| block_002 | -                   |
```

**`read <user>` 输出：**
```
User: client_1
Blocks: 0-0
Content: "Hello World"
Size: 11 bytes
```

## 项目结构

```
Shared-Memory-Manage-System/
├── server/
│   ├── main.cpp                          # 主程序入口，REPL 循环
│   ├── command/
│   │   ├── commands.h                    # 命令处理声明
│   │   └── commands.cpp                  # 命令处理实现
│   ├── shared_memory_pool/
│   │   ├── shared_memory_pool.h          # 内存池类声明
│   │   └── shared_memory_pool.cpp        # 内存池实现
│   ├── persistence/
│   │   ├── persistence.h                 # 持久化模块声明
│   │   └── persistence.cpp               # 持久化模块实现（开发中）
│   ├── run.bat                           # 编译运行脚本
│   └── build.bat                         # 编译脚本
├── details.md                            # 项目需求文档
└── README.md                             # 本文件
```

## 技术栈

- **语言**：C++17
- **编译器**：g++ (MinGW-w64 / MSYS2)
- **标准库**：STL（`vector`, `map`, `bitset`, `array`）
- **平台**：Windows

## 代码质量

- ✅ 遵循 C++17 标准
- ✅ 使用现代 C++ 特性（`constexpr`, `auto`, 范围 for 循环）
- ✅ 内存安全（使用 STL 容器，避免裸指针）
- ✅ 错误处理（返回值检查，异常安全）
- ✅ 代码格式化（clang-format，K&R 风格）
- ✅ 模块化设计（功能分离到不同目录）

## 开发进度

| 功能模块   | 状态     | 完成度 |
| ---------- | -------- | ------ |
| 内存池核心 | ✅ 完成   | 100%   |
| 内存分配   | ✅ 完成   | 100%   |
| 内存释放   | ✅ 完成   | 100%   |
| 内存紧凑   | ✅ 完成   | 100%   |
| 命令行界面 | ✅ 完成   | 100%   |
| 状态查询   | ✅ 完成   | 100%   |
| 内容读取   | ✅ 完成   | 100%   |
| 批量执行   | ✅ 完成   | 100%   |
| 数据持久化 | ✅ 完成   | 100%   |
| TCP 服务器 | ⏳ 待实现 | 0%     |
| 客户端程序 | ⏳ 待实现 | 0%     |

## 待实现功能

### 短期目标
- [x] 完成数据持久化功能（保存/加载内存池状态）
- [x] 实现批量执行功能（`exec` 命令）
- [ ] TCP 服务器功能（监听端口，接受客户端连接）
- [ ] 客户端程序（`client.exe`）
- [ ] 网络协议实现（请求/响应格式）

### 长期目标
- [ ] 支持多客户端并发访问
- [ ] 内存池大小可配置（当前固定 1MB）
- [ ] 性能优化（减少内存拷贝，优化紧凑算法）
- [ ] 日志系统
- [ ] 单元测试

## 性能指标

- **内存池大小**：1MB（256 × 4KB 块）
- **分配算法**：首次适配（First Fit）+ 自动紧凑
- **时间复杂度**：
  - 分配：O(n) 最坏情况，n 为块数
  - 紧凑：O(n)
  - 释放：O(1)
  - 查询：O(1) 到 O(n)

## 注意事项

1. 当前版本为服务器端核心功能实现，TCP 网络功能尚未集成
2. 数据持久化功能已完成，支持程序退出时自动保存状态（Ctrl+C、Ctrl+Z、quit/exit）
3. 内存池大小可在 `shared_memory_pool/shared_memory_pool.h` 中通过 `kPoolSize` 常量调整
4. 建议使用支持 C++17 的编译器（g++ 7.0+ 或 MSVC 2017+）
5. 块 ID 显示格式为 3 位数字，不足 3 位用 0 填充（如 `block_000`, `block_030`）
6. 持久化文件 `memory_pool.dat` 保存在服务器程序运行目录

## 更新日志

### v0.3.1 (当前版本)
- ✅ 实现 `exec` 命令，支持从文件批量执行命令
- ✅ 实现 `free`/`delete` 命令，支持释放用户内存
- ✅ 实现 `update` 命令，支持更新用户内容
- ✅ 修复 `compact` 后元数据清理问题
- ✅ 优化 `status --block` 显示逻辑（检查 `used` 状态）

### v0.3.0
- ✅ 完成数据持久化功能（保存/加载内存池状态）
- ✅ 实现信号处理（Ctrl+C、Ctrl+Z 自动保存）
- ✅ 优化 `status --memory` 显示顺序（按起始 block 排序）
- ✅ 移除 `BlockMeta` 中的 `content` 字段，简化数据结构

### v0.2.0
- ✅ 重构项目结构，采用模块化设计
- ✅ 实现 `read` 命令，支持读取用户内容
- ✅ 优化状态显示格式（块 ID 3 位数字格式）
- 🚧 开始实现数据持久化功能

### v0.1.0
- ✅ 实现内存池核心功能
- ✅ 实现内存分配、释放、紧凑
- ✅ 实现命令行界面和状态查询
- ✅ 完成代码重构和错误修复
- ✅ 通过编译和基本功能测试

---

**项目状态**：核心功能和持久化已完成，TCP 网络功能待实现 ⏳
