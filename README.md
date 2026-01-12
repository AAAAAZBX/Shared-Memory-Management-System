# Shared Memory Management System

## 项目简介

本项目是一个基于 C++17 实现的共享内存管理系统，采用固定块大小（4KB）的内存池管理策略，提供高效的内存分配、释放和紧凑功能。系统设计通过 TCP 服务器提供客户端接口，支持多客户端通过网络连接访问共享内存池（TCP 客户端接口功能待实现）。

## 核心特性

### 内存管理
- **固定块大小管理**：将内存池划分为 256 个 4KB 的固定大小块（当前配置为 100MB 内存池）
- **连续块分配**：支持跨多个块的数据存储，自动计算所需块数
- **内存紧凑（Compaction）**：当找不到连续空闲块但总空间足够时，自动进行内存紧凑，合并碎片
- **Memory ID 标识**：使用 `memory_00001`, `memory_00002` 等唯一标识符区分不同的内存块
- **内容描述**：每个内存块都有描述信息，方便识别和管理

### 已实现功能

#### 1. 内存池初始化与重置
- 自动分配 100MB 连续内存空间（25,600 个 4KB 块）
- 支持内存池重置，清空所有分配信息

#### 2. 内存分配（`alloc`）
- `alloc "<description>" "<content>"`：分配内存并指定描述信息
- 自动生成 Memory ID（`memory_00001`, `memory_00002`, ...）
- 自动计算所需块数（向上取整）
- 空间不足时自动触发紧凑操作
- 返回分配的 Memory ID 和起始块 ID

#### 3. 内存释放（`free` / `delete`）
- `free <memory_id>`：释放指定 Memory ID 的所有内存块
- `delete <memory_id>`：释放指定 Memory ID 的所有内存块（`free` 的别名）

#### 4. 内容更新（`update`）
- `update <memory_id> "<new_content>"`：更新指定 Memory ID 的内存内容
- 如果新内容大小超过原分配，自动重新分配
- 如果新内容大小不超过原分配，直接覆盖写入
- 保持原有的 Memory ID 和描述信息

#### 5. 内存紧凑（`compact`）
- 自动合并碎片，将已使用的块移动到内存池前端
- 更新内存块信息映射关系

#### 6. 状态查询（`status`）
- `status --memory`：显示内存池使用情况，按 Memory ID 展示占用范围（格式：`block_000 - block_015`），包含 Memory ID、描述和最后修改时间
- `status --block`：只显示已使用的块，包括 Memory ID、描述和最后修改时间（格式：`block_000`, `block_001` 等）

#### 7. 内容读取（`read`）
- `read <memory_id>`：显示指定 Memory ID 的内容
- 直接从内存池读取，遇到 0 停止
- 显示 Memory ID、描述、块范围、内容和大小

#### 8. 命令行界面
- 交互式 REPL（Read-Eval-Print Loop）界面
- 完整的帮助系统（`help` 命令）
- 优雅的退出机制（`quit` / `exit`）

#### 9. 数据持久化
- 自动保存内存池状态到 `memory_pool.dat` 文件
- 支持程序正常退出（`quit`/`exit`）时保存
- 支持异常退出（Ctrl+C、Ctrl+Z）时自动保存
- 程序启动时自动加载之前保存的状态
- 二进制文件格式（版本2），包含文件头、元数据、使用状态位图、用户块信息、用户最后修改时间和内存池数据
- 支持版本兼容：可以加载版本1的旧文件（时间信息为空）

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
- 内存分配信息映射（`map<string, pair<size_t, size_t>>`，key 为 Memory ID）

#### 关键数据结构
```cpp
struct BlockMeta {
    bool used;              // 是否被使用
    std::string memory_id;  // 内存ID（memory_00001, memory_00002, ...）
    std::string description; // 内容描述
};
```
注意：系统还维护 `memory_last_modified_time` 映射（`map<string, time_t>`），记录每个 Memory ID 最后修改内存的时间戳，用于在 `status` 命令中显示。

### 内存布局
```
┌─────────────────────────────────────────┐
│  Memory Pool (100MB = 25,600 × 4KB blocks)  │
├─────────────────────────────────────────┤
│ Block 0   │ Block 1   │ ... │ Block 25599 │
│ (4KB)     │ (4KB)     │     │ (4KB)       │
└─────────────────────────────────────────┘
```

### 数据持久化文件格式

程序退出时会自动将内存池状态保存到 `memory_pool.dat` 文件中，启动时会自动加载。文件格式如下：

#### 文件结构（按顺序）

```
┌─────────────────────────────────────────────────────────┐
│ 1. FileHeader (文件头)                                    │
│    - magic: uint32_t (0x4D454D50, "MEMP")                │
│    - version: uint32_t (当前版本: 3)                      │
│    - free_block_count: size_t (空闲块数量)                │
│    - memory_info_count: size_t (内存信息条目数)           │
│    - reserved: uint64_t[4] (预留字段)                    │
├─────────────────────────────────────────────────────────┤
│ 2. BlockMeta Array (元数据数组，25,600个块)              │
│    对每个块 i (0-255):                                    │
│    - used: bool (是否被使用)                              │
│    - memoryIdLen: size_t (Memory ID 长度)                │
│    - memory_id: char[memoryIdLen] (Memory ID 字符串)     │
│    - descLen: size_t (描述长度)                          │
│    - description: char[descLen] (描述字符串)             │
├─────────────────────────────────────────────────────────┤
│ 3. Used Map (使用状态位图)                                │
│    - bitsetBytes: uint8_t[(256+7)/8] (32字节)            │
│      (每个位表示对应块的使用状态)                          │
├─────────────────────────────────────────────────────────┤
│ 4. Memory Info (内存块信息映射)                           │
│    对每个内存条目:                                        │
│    - keyLen: size_t (Memory ID 长度)                    │
│    - key: char[keyLen] (Memory ID 字符串)               │
│    - startBlock: size_t (起始块ID)                       │
│    - blockCount: size_t (块数量)                         │
├─────────────────────────────────────────────────────────┤
│ 5. Memory Last Modified Time (最后修改时间映射)          │
│    对每个内存条目:                                        │
│    - keyLen: size_t (Memory ID 长度)                    │
│    - key: char[keyLen] (Memory ID 字符串)               │
│    - timeValue: time_t (最后修改时间戳)                  │
├─────────────────────────────────────────────────────────┤
│ 6. Pool Data (内存池数据)                                 │
│    - pool_: uint8_t[kPoolSize] (内存池原始数据)          │
└─────────────────────────────────────────────────────────┘
```

#### 文件格式说明

- **文件类型**：二进制文件
- **字节序**：小端序（Little-Endian，Windows 默认）
- **魔数**：`0x4D454D50`（ASCII: "MEMP"），用于文件格式验证
- **版本号**：当前为 `3`，用于未来格式升级兼容
- **版本兼容**：支持加载版本1和版本2的旧文件（向后兼容）

#### 文件大小估算

- 文件头：`sizeof(FileHeader)` ≈ 48 字节
- 元数据数组：256 × (1 + 8 + 8 + 可变字符串长度) ≈ 4-15 KB（取决于字符串长度）
- Used Map：32 字节
- Memory Info：可变（取决于内存块数量）
- Memory Last Modified Time：可变（取决于内存块数量）
- Pool Data：kPoolSize 字节（当前为 100MB）
- **总大小**：约 100MB+（取决于元数据大小）

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

# 分配内存（自动生成 Memory ID）
server> alloc "User Data" "Hello World"
Allocation successful. Memory ID: memory_00001
Description: User Data
Content stored at block 0

# 读取内存内容
server> read memory_00001

# 释放内存
server> free memory_00001
server> delete memory_00001  # free 的别名

# 更新内存内容
server> update memory_00001 "Updated Content"

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
| range                 | MemoryID     | Description | Last Modified       |
| --------------------- | ------------ | ----------- | ------------------- |
| block_000 - block_015 | memory_00001 | User Data   | 2024-01-15 10:30:45 |
| block_016 - block_031 | memory_00002 | Config Data | 2024-01-15 11:20:10 |
```

**`status --block` 输出：**
```
Block Pool Status:
| ID        | MemoryID     | Description | Last Modified       |
| --------- | ------------ | ----------- | ------------------- |
| block_000 | memory_00001 | User Data   | 2024-01-15 10:30:45 |
| block_001 | memory_00001 | User Data   | 2024-01-15 10:30:45 |
```

**`read <memory_id>` 输出：**
```
Memory ID: memory_00001
Description: User Data
Blocks: 0-0
Content: "Hello World"
Size: 11 bytes
Last Modified: 2024-01-15 10:30:45
```

## 项目结构

```
Shared-Memory-Manage-System/
├── server/                               # 服务器端程序
│   ├── main.cpp                          # 主程序入口，REPL 循环
│   ├── command/                          # 命令处理模块
│   │   ├── commands.h                    # 命令处理声明
│   │   └── commands.cpp                  # 命令处理实现（支持 Memory ID 系统）
│   ├── shared_memory_pool/               # 内存池核心模块
│   │   ├── shared_memory_pool.h          # 内存池类声明（Memory ID 系统）
│   │   └── shared_memory_pool.cpp        # 内存池实现
│   ├── persistence/                      # 持久化模块
│   │   ├── persistence.h                 # 持久化模块声明
│   │   └── persistence.cpp               # 持久化实现（版本3，支持 Memory ID）
│   ├── network/                           # 网络模块（TCP 客户端接口，待实现）
│   │   ├── tcp_server.h                  # TCP 服务器声明
│   │   ├── tcp_server.cpp                # TCP 服务器实现（待更新为 Memory ID 系统）
│   │   ├── protocol.h                    # 网络协议定义
│   │   ├── protocol.cpp                  # 协议编解码实现
│   │   └── README_TCP.md                 # TCP 客户端接口设计说明
│   ├── run.bat                           # 编译运行脚本
│   └── build.bat                         # 编译脚本
├── doc.md                                # 技术文档（面向开发者）
├── README.md                             # 项目说明文档（本文件）
├── details.md                            # 项目需求文档
├── problem.txt                           # 问题记录
├── sample.txt                            # 命令示例文件
└── .gitignore                            # Git 忽略配置
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

| 功能模块       | 状态     | 完成度 |
| -------------- | -------- | ------ |
| 内存池核心     | ✅ 完成   | 100%   |
| 内存分配       | ✅ 完成   | 100%   |
| 内存释放       | ✅ 完成   | 100%   |
| 内存紧凑       | ✅ 完成   | 100%   |
| 命令行界面     | ✅ 完成   | 100%   |
| 状态查询       | ✅ 完成   | 100%   |
| 内容读取       | ✅ 完成   | 100%   |
| 批量执行       | ✅ 完成   | 100%   |
| 数据持久化     | ✅ 完成   | 100%   |
| TCP 服务器接口 | ⏳ 待实现 | 0%     |

## 待实现功能

### 短期目标
- [x] 完成数据持久化功能（保存/加载内存池状态）
- [x] 实现批量执行功能（`exec` 命令）
- [ ] TCP 服务器功能（监听端口，提供客户端接口）
- [ ] 网络协议实现（请求/响应格式，支持 Memory ID 系统）
- [ ] 客户端接口实现（通过 TCP 协议访问共享内存池）

### 长期目标
- [ ] 内存池大小可配置（当前固定 100MB）
- [ ] 性能优化（减少内存拷贝，优化紧凑算法）
- [ ] 日志系统
- [ ] 单元测试

## 性能指标

- **内存池大小**：100MB（25,600 × 4KB 块）
- **分配算法**：首次适配（First Fit）+ 自动紧凑
- **时间复杂度**：
  - 分配：O(n) 最坏情况，n 为块数
  - 紧凑：O(n)
  - 释放：O(1)
  - 查询：O(1) 到 O(n)

## 注意事项

1. **客户端接口**：系统通过 TCP 服务器提供客户端接口，客户端通过网络连接访问共享内存池，无需单独的客户端程序
2. **Memory ID 系统**：使用 `memory_00001`, `memory_00002` 等唯一标识符区分内存块，所有客户端共享访问
3. **数据持久化**：功能已完成，支持程序退出时自动保存状态（Ctrl+C、Ctrl+Z、quit/exit）
4. **内存池大小**：可在 `shared_memory_pool/shared_memory_pool.h` 中通过 `kPoolSize` 常量调整（当前为 100MB）
5. **编译器要求**：建议使用支持 C++17 的编译器（g++ 7.0+ 或 MSVC 2017+）
6. **块 ID 格式**：显示格式为 3 位数字，不足 3 位用 0 填充（如 `block_000`, `block_030`）
7. **持久化文件**：`memory_pool.dat` 保存在服务器程序运行目录，不应提交到版本控制（已在 `.gitignore` 中配置）

## 更新日志

### v0.4.0 (当前版本)
- ✅ **重大架构变更**：取消用户身份系统，改为 Memory ID 系统
- ✅ 使用 `memory_00001`, `memory_00002` 等唯一标识符区分内存块
- ✅ 每个内存块都有描述信息（Description）
- ✅ `alloc` 命令改为 `alloc "<description>" "<content>"`，自动生成 Memory ID
- ✅ `read`/`free`/`update` 命令改为使用 Memory ID
- ✅ `status` 命令显示 Memory ID 和 Description 而不是 Occupied Client
- ✅ `status --block` 只显示已使用的块，不显示空块
- ✅ 持久化功能升级到版本3，支持保存和加载 Memory ID 和描述信息
- ✅ 支持版本兼容：可以加载版本1和版本2的旧文件（向后兼容）

### v0.3.2
- ✅ 添加用户最后修改时间追踪功能
- ✅ `status --memory` 和 `status --block` 命令显示最后修改时间
- ✅ 持久化功能升级到版本2，支持保存和加载时间信息
- ✅ 支持版本兼容：可以加载版本1的旧文件（时间信息为空）
- ✅ 时间信息在 `alloc` 和 `update` 操作时自动更新

### v0.3.1
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

**项目状态**：核心功能、持久化和时间追踪已完成，TCP 客户端接口待实现 ⏳
