# Shared Memory Management System - 技术文档

## 文档目录

- [Shared Memory Management System - 技术文档](#shared-memory-management-system---技术文档)
  - [文档目录](#文档目录)
  - [0. 项目概述](#0-项目概述)
    - [0.1 项目简介](#01-项目简介)
      - [核心特性](#核心特性)
      - [技术栈](#技术栈)
      - [项目状态](#项目状态)
    - [0.2 目录结构](#02-目录结构)
    - [0.3 文件功能说明](#03-文件功能说明)
      - [服务器端核心文件](#服务器端核心文件)
  - [1. 系统架构](#1-系统架构)
    - [1.1 整体架构](#11-整体架构)
    - [1.2 模块划分](#12-模块划分)
      - [内存池模块 (`core/shared_memory_pool`)](#内存池模块-coreshared_memory_pool)
      - [命令处理模块 (`command`)](#命令处理模块-command)
      - [持久化模块 (`core/persistence`)](#持久化模块-corepersistence)
      - [网络模块 (`network`)](#网络模块-network)
    - [1.3 数据流](#13-数据流)
      - [命令执行流程](#命令执行流程)
      - [内存分配流程](#内存分配流程)
      - [持久化流程](#持久化流程)
  - [2. 核心模块与API](#2-核心模块与api)
    - [2.1 内存池模块](#21-内存池模块)
      - [核心常量](#核心常量)
      - [关键API](#关键api)
      - [实现要点](#实现要点)
    - [2.2 命令处理模块](#22-命令处理模块)
      - [命令定义结构](#命令定义结构)
      - [支持的命令](#支持的命令)
      - [核心函数](#核心函数)
    - [2.3 持久化模块](#23-持久化模块)
      - [文件格式](#文件格式)
      - [核心API](#核心api)
    - [2.4 网络模块](#24-网络模块)
      - [TCP 服务器设计 ✅](#tcp-服务器设计-)
      - [客户端实现指南 ✅](#客户端实现指南-)
  - [3. 数据结构与算法](#3-数据结构与算法)
    - [3.1 数据结构](#31-数据结构)
      - [BlockMeta 结构](#blockmeta-结构)
      - [内存池布局](#内存池布局)
      - [核心数据结构](#核心数据结构)
    - [3.2 核心算法](#32-核心算法)
      - [Memory ID 生成算法（Base62 编码）](#memory-id-生成算法base62-编码)
      - [内存分配算法](#内存分配算法)
      - [内存紧凑算法](#内存紧凑算法)
      - [位图优化](#位图优化)
  - [4. 开发指南](#4-开发指南)
    - [4.1 代码规范](#41-代码规范)
      - [命名规范](#命名规范)
      - [代码格式](#代码格式)
    - [4.2 扩展开发](#42-扩展开发)
      - [添加新命令](#添加新命令)
      - [修改内存池大小 ✅（已完成扩展至 1GB）](#修改内存池大小-已完成扩展至-1gb)
      - [添加持久化字段](#添加持久化字段)
      - [文件上传功能实现](#文件上传功能实现)
      - [UTF-8 中文支持](#utf-8-中文支持)
  - [附录](#附录)
    - [性能指标](#性能指标)
    - [常见问题](#常见问题)

---

## 0. 项目概述

### 0.1 项目简介

**Shared Memory Management System** 是一个基于 C++17 实现的共享内存管理系统，采用固定块大小（4KB）的内存池管理策略。

#### 核心特性

- **固定块管理**：将内存池划分为 262,144 个 4KB 的固定大小块（当前配置为 1GB 内存池）
- **连续块分配**：支持跨多个块的数据存储，自动计算所需块数
- **内存紧凑**：当找不到连续空闲块但总空间足够时，自动进行内存紧凑，合并碎片
- **数据持久化**：支持程序退出时自动保存状态，启动时自动恢复
- **命令行界面**：提供交互式 REPL 界面，支持多种命令操作

#### 技术栈

- **语言**：C++17
- **编译器**：g++ (MinGW-w64 / MSYS2)
- **标准库**：STL（`vector`, `map`, `bitset`, `array`）
- **平台**：Windows

#### 项目状态

- ✅ **已完成**：内存池核心、命令处理、数据持久化、文件上传功能、UTF-8 中文支持、表格显示优化、内存池扩展（1GB）
- 🔄 **进行中**：TCP 服务器（60%）、客户端接口规范（60%）
- ⏳ **计划中**：紧凑算法优化、SDK/DLL 打包
- ⚠️ **已知问题**：TCP 连接功能存在一些问题，正在修复中

### 0.2 目录结构

```
Shared-Memory-Manage-System/
├── core/                           # 核心库（未来可编译成 DLL）
│   ├── shared_memory_pool/        # 内存池核心模块
│   │   ├── shared_memory_pool.h   # 内存池类声明
│   │   └── shared_memory_pool.cpp # 内存池类实现
│   ├── persistence/                # 持久化模块
│   │   ├── persistence.h          # 持久化接口声明
│   │   └── persistence.cpp         # 持久化实现
│   └── api/                        # C API 包装层
│       ├── smm_api.h              # C API 头文件
│       └── smm_api.cpp            # C API 实现
├── server/                         # 服务器端程序（使用 core/）
│   ├── main.cpp                   # 服务器主程序入口
│   ├── command/                    # 命令处理模块
│   │   ├── commands.h             # 命令处理声明
│   │   └── commands.cpp           # 命令处理实现
│   ├── network/                    # 网络模块
│   │   ├── tcp_server.h           # TCP 服务器声明
│   │   ├── tcp_server.cpp         # TCP 服务器实现
│   │   ├── protocol.h             # 网络协议定义
│   │   ├── protocol.cpp           # 协议编解码实现
│   │   └── README_TCP.md          # TCP 客户端接口设计说明
│   ├── MEMORY_POOL_SIZE_GUIDE.md  # 内存池大小配置指南
│   ├── run.bat                    # 编译运行脚本
│   └── build.bat                  # 编译脚本
├── sdk/                            # SDK 发布包结构
│   ├── include/                   # 头文件目录
│   ├── lib/                       # 库文件目录
│   ├── docs/                      # 文档目录
│   ├── examples/                  # 示例代码目录
│   │   └── basic_usage.cpp        # C API 基础使用示例
│   └── README.md                  # SDK 说明
├── client/                         # 客户端实现
│   ├── client.py                  # 客户端参考实现
│   └── README.md                  # 客户端实现指南（适用于所有语言）
├── sample/                         # 示例文件目录
│   ├── env.txt                    # 6KB 文件示例
│   ├── error.txt                  # 153KB 文件示例
│   └── 测试中文.txt               # 中文测试文件
├── README.md                       # 项目说明文档（面向用户）
├── doc.md                          # 技术文档（面向开发者，本文件）
├── details.md                      # 项目需求文档
├── DLL_SDK_GUIDE.md               # DLL/SDK 打包指南
└── JAVA_GC_REFERENCE.md           # Java GC 原理参考
```

### 0.3 文件功能说明

#### 服务器端核心文件

- **`main.cpp`**：程序入口，初始化内存池，启动 REPL 循环，处理信号
- **`core/shared_memory_pool/`**：内存池核心，管理 1GB 内存空间，提供分配/释放/紧凑功能
- **`core/persistence/`**：持久化，保存/加载内存池状态到文件
- **`core/api/`**：C API 包装层，提供跨语言的 C 接口
- **`server/command/`**：命令处理，解析用户命令，调用内存池接口
- **`server/network/`**：网络模块，TCP 服务器实现，支持多客户端并发连接
- **`client/`**：客户端实现指南和参考实现
- **`sdk/`**：SDK 发布包结构，包含头文件、库文件、文档和示例


---

## 1. 系统架构

### 1.1 整体架构

本系统采用分层模块化设计：

```
┌─────────────────────────────────────────┐
│         用户交互层 (REPL/Network)        │
├─────────────────────────────────────────┤
│         命令处理层 (Command Handler)     │
├─────────────────────────────────────────┤
│         业务逻辑层 (Memory Pool)         │
├─────────────────────────────────────────┤
│         持久化层 (Persistence)           │
└─────────────────────────────────────────┘
```

**设计原则**：
- **单一职责**：每个模块只负责一个功能领域
- **低耦合**：模块间通过接口交互
- **高内聚**：相关功能集中在同一模块

### 1.2 模块划分

#### 内存池模块 (`core/shared_memory_pool`)
- **职责**：管理固定大小的内存块分配
- **核心类**：`SharedMemoryPool`
- **数据结构**：`pool_`（1GB内存）、`meta_`（元数据数组，动态分配）、`used_map`（位图）、`memory_info`（内存块映射表）
- **位置**：`core/shared_memory_pool/`（已从 `server/` 移至 `core/`）

#### 命令处理模块 (`command`)
- **职责**：解析和执行用户命令
- **核心函数**：`HandleCommand()`
- **支持命令**：help、status、alloc、read、free/delete、update、exec、compact、reset、quit、info
- **Memory ID 系统**：使用 `memory_00001`, `memory_00002` 等唯一标识符，不再使用用户身份

#### 持久化模块 (`core/persistence`)
- **职责**：序列化/反序列化内存池状态
- **核心函数**：`Persistence::Save()` / `Load()`
- **文件格式**：二进制格式（版本3），包含文件头、元数据、位图、内存块信息、最后修改时间、内存池数据
- **版本兼容**：支持加载版本1和版本2的旧文件
- **位置**：`core/persistence/`（已从 `server/` 移至 `core/`）

#### 网络模块 (`network`)
- **职责**：TCP 服务器，提供客户端接口，处理客户端连接
- **核心类**：`TCPServer`
- **协议**：自定义二进制协议
- **客户端接口**：系统通过 TCP 服务器提供客户端接口，客户端通过网络连接访问，无需单独的程序
- **Memory ID 系统**：使用 Memory ID 标识内存块，所有客户端共享访问

### 1.3 数据流

#### 命令执行流程
```
用户输入 → 命令解析 → 参数验证 → 调用内存池接口 → 返回结果 → 格式化输出
```

#### 内存分配流程
```
AllocateBlock() → 计算所需块数 → 查找连续空闲块 → 
[未找到] → Compact() → 重新查找 → 
[找到] → 写入数据 → 更新元数据 → 返回块ID
```

#### 持久化流程
```
保存：程序退出 → SignalHandler → Persistence::Save() → 序列化所有数据 → 写入文件
加载：程序启动 → Persistence::Load() → 读取文件 → 验证格式 → 反序列化 → 恢复状态
```

---

## 2. 核心模块与API

### 2.1 内存池模块

#### 核心常量
```cpp
static constexpr size_t kPoolSize = 1024 * 1024 * 1024;  // 1GB
static constexpr size_t kBlockSize = 4096;               // 4KB
static constexpr size_t kBlockCount = kPoolSize / kBlockSize;  // 262,144 块
```

#### 关键API

**初始化**
- `bool Init()`：分配 1MB 连续内存，初始化所有元数据
- `void Reset()`：清空所有数据，恢复到初始状态

**内存分配**
- `int AllocateBlock(const std::string& memory_id, const std::string& description, const void* data, size_t dataSize)`
  - 为指定 Memory ID 分配内存并写入数据
  - 返回：起始块 ID（成功）或 -1（失败）
  - 自动计算所需块数，空间不足时触发紧凑
- `std::string GenerateNextMemoryId()`：生成下一个可用的 Memory ID（O(1) 时间复杂度）
  - 使用 Base62 编码，格式：`memory_xxxxx`（xxxxx 是 Base62 编码）
  - 5位支持约9亿个ID，6位支持约568亿个ID（自动扩展）
  - 向后兼容旧的纯数字格式
- `void InitializeMemoryIdCounter()`：初始化 Memory ID 计数器（从文件加载后调用）

**内存释放**
- `bool FreeByMemoryId(const std::string& memory_id)`：释放指定 Memory ID 的所有内存
- `bool FreeByBlockId(size_t blockId)`：释放指定块的内存

**内容更新**
- 通过 `update` 命令实现，支持两种模式：
  - 新内容大小 ≤ 原分配：直接覆盖写入，剩余部分清零
  - 新内容大小 > 原分配：先释放原内存，再重新分配（保持相同的 Memory ID 和描述）

**查询接口**
- `const BlockMeta& GetMeta(size_t blockId)`：获取块的元数据
- `const std::map<std::string, std::pair<size_t, size_t>>& GetMemoryInfo()`：获取内存块映射表
- `std::string GetMemoryContentAsString(const std::string& memory_id)`：读取内存内容（遇到0停止）
- `time_t GetMemoryLastModifiedTime(const std::string& memory_id)`：获取内存最后修改时间戳
- `std::string GetMemoryLastModifiedTimeString(const std::string& memory_id)`：获取内存最后修改时间字符串（格式：YYYY-MM-DD HH:MM:SS）

**工具方法**
- `void Compact()`：紧凑内存，合并碎片，将所有已使用的块移动到前端
  - 修复了多块内存的起始位置更新问题
  - 使用 `newStartPositions` 映射确保每个 memory_id 只更新一次起始位置
  - 确保 compact 后所有已使用的块连续排列，不留空隙

#### 实现要点

- **分配策略**：首次适配算法，找不到连续块时自动紧凑
- **紧凑算法**：O(n) 时间复杂度，原地操作
- **时间复杂度**：分配 O(n)，释放 O(1)，查询 O(1)

### 2.2 命令处理模块

#### 命令定义结构
```cpp
struct CommandSpec {
    std::string name;                    // 命令名称
    std::string summary;                 // 简要说明
    std::string usage;                   // 使用格式
    std::vector<std::string> examples;   // 示例
};
```

#### 支持的命令

- **help [command]**：显示帮助信息
- **status [--memory|--block]**：显示内存池状态
  - `status --memory`：显示 Memory ID、Description、Bytes（实际数据大小）、Range（格式：`block_000 - block_015(16 blocks, 64KB)`，包含块数和大小信息）、Last Modified
  - `status --block`：显示已使用的块信息
  - 支持中文显示，表格列自动对齐（中文字符按 2 个显示宽度计算）
- **info**：显示系统综合信息
- **alloc "<description>" "<content>"**：分配内存，自动生成 Memory ID
- **alloc "<description>" @<filepath>**：从文件上传内容到内存池（支持 .txt 文件）
  - 支持相对路径和绝对路径
  - 支持带引号的路径：`@"path/to/file.txt"` 或 `@"C:\Users\file.txt"`
  - 自动检测和移除 UTF-8 BOM
- **read <memory_id>**：读取指定 Memory ID 的内容
  - 使用虚线分隔元信息和内容
  - 内容原样输出，不添加引号
  - 支持多行内容和中文显示
- **free <memory_id>**：释放指定 Memory ID 的内存
- **delete <memory_id>**：释放指定 Memory ID 的内存（`free` 的别名）
- **update <memory_id> "<new_content>"**：更新指定 Memory ID 的内容
- **exec <filename>**：从文件批量执行命令
- **compact**：紧凑内存
  - 将所有已使用的块移动到内存池前端
  - 修复了多块内存的起始位置更新问题
  - 确保 compact 后所有已使用的块连续排列
- **reset**：重置内存池（需要密码确认）
- **quit/exit**：退出程序

#### 核心函数

**void HandleCommand(const std::vector<std::string>& tokens, SharedMemoryPool& smp)**
- 解析命令和参数
- 调用相应的内存池接口
- 格式化输出结果

**exec 命令特性**
- 从文件逐行读取命令并执行
- 支持空行和注释（以 `#` 开头）
- 显示执行的行号和命令内容
- 遇到 `quit`/`exit` 时停止执行（不退出程序）
- 自动去除行首尾空白字符

### 2.3 持久化模块

#### 文件格式

**文件头结构**（48 字节）：
```cpp
struct FileHeader {
    uint32_t magic;             // 魔数 "MEMP" (0x4D454D50)
    uint32_t version;           // 版本号 (1: 基础版本, 2: 包含时间信息, 3: Memory ID 系统)
    size_t free_block_count;    // 空闲块数量
    size_t memory_info_count;  // 内存信息条目数（版本3），或 user_info_count（版本1/2）
    uint64_t reserved[4];       // 预留字段
};
```

**文件内容顺序**：
1. 文件头（48 字节）
2. 元数据数组（kBlockCount 个块）
   - 版本3：包含 memory_id 和 description
   - 版本1/2：包含 user（向后兼容）
3. 使用状态位图
4. 内存块信息映射（版本3）或用户块信息映射（版本1/2）
5. 最后修改时间映射（版本2/3）
6. 内存池数据（kPoolSize 字节，当前为 1GB）

**版本兼容性**：
- 版本1：基础版本，不包含时间信息
- 版本2：包含用户最后修改时间信息，支持向后兼容（可加载版本1文件）
- 版本3：使用 Memory ID 和描述系统，支持向后兼容（可加载版本1和版本2文件）

#### 核心API

**bool Persistence::Save(const SharedMemoryPool& smp, const std::string& filename)**
- 序列化内存池状态到文件
- 返回：true（成功）或 false（失败）

**bool Persistence::Load(SharedMemoryPool& smp, const std::string& filename)**
- 从文件加载内存池状态
- 返回：true（成功）或 false（失败/文件不存在）

### 2.4 网络模块

#### TCP 服务器设计 ✅

**设计目标**：
- 通过 TCP 服务器提供客户端接口
- 客户端通过网络连接访问共享内存池
- 支持多客户端并发访问
- 使用 Memory ID 系统管理内存（所有客户端共享访问）

**核心功能**：
- ✅ 监听指定端口（默认8888）
- ✅ 接受客户端连接
- ✅ 为每个连接创建独立处理线程
- ✅ 使用 Memory ID 系统管理内存
- ✅ 处理客户端请求并返回响应
- ✅ 服务器启动时显示可访问的 IP 地址和端口

**网络协议**：
- **请求格式**：`[命令类型: 1字节] [数据长度: 4字节(大端序)] [数据内容: N字节]`
- **响应格式**：`[状态码: 1字节] [数据长度: 4字节(大端序)] [响应数据: N字节]`
- **命令类型**：
  - `ALLOC (0x01)`: 分配内存，数据格式为 `description\0content`，返回 Memory ID
  - `UPDATE (0x02)`: 更新内容，数据格式为 `memory_id\0new_content`
  - `DELETE (0x03)`: 删除内存，数据格式为 `memory_id`
  - `READ (0x04)`: 读取内容，数据格式为 `memory_id`，返回格式化的内存信息
  - `STATUS (0x05)`: 查询状态，无数据，返回所有内存块的状态信息
  - `PING (0x06)`: 心跳检测，无数据，返回 "PONG"
- **状态码**：
  - `SUCCESS (0x00)`: 成功
  - `ERROR_INVALID_CMD (0x01)`: 无效命令
  - `ERROR_INVALID_PARAM (0x02)`: 无效参数
  - `ERROR_NO_MEMORY (0x03)`: 内存不足
  - `ERROR_NOT_FOUND (0x04)`: Memory ID 不存在
  - `ERROR_INTERNAL (0xFF)`: 内部错误

**实现状态**：
- ✅ TCP 服务器完整实现（`tcp_server.h/cpp`）
- ✅ 网络协议完整实现（`protocol.h/cpp`）
- ✅ 已更新为 Memory ID 系统
- ✅ 已集成到主程序（`main.cpp`）
- ✅ 支持 Windows 平台（已移除 Linux/Unix 相关代码）

#### 客户端实现指南 ✅

**位置**：`client/README.md`

**内容**：
- ✅ 完整的协议规范说明
- ✅ 适用于所有编程语言的实现指南
- ✅ 多语言实现示例（C/C++、Python、Java、Go 等）
- ✅ 操作示例和错误处理说明
- ✅ 测试建议和故障排除指南

**参考实现**：`client/client.py`

项目提供了客户端参考实现，包含：
- 完整的协议封装
- 所有操作的 API
- 错误处理
- 上下文管理器支持

**客户端实现要求**：
- 支持 TCP Socket 的编程语言
- 实现协议编解码（命令类型、数据长度、数据内容）
- 处理字节序转换（大端序）
- 实现错误处理

详细实现指南请参考 `client/README.md`。

---

## 3. 数据结构与算法

### 3.1 数据结构

#### BlockMeta 结构
```cpp
struct BlockMeta {
    bool used = false;           // 是否被使用
    std::string memory_id = "";  // 内存ID（memory_00001, memory_00002, ...）
    std::string description = ""; // 内容描述
};
```

**时间信息管理**：
系统还维护 `memory_last_modified_time` 映射（`std::map<std::string, time_t>`），记录每个 Memory ID 最后修改内存的时间戳。该信息在 `alloc`、`update` 操作时自动更新，并在 `status` 命令中显示。

#### 内存池布局
```
┌─────────────────────────────────────────────────────────┐
│                    Memory Pool (1GB)                     │
├─────────────────────────────────────────────────────────┤
│ Block 0 (4KB) │ Block 1 (4KB) │ ... │ Block 262143 (4KB) │
└─────────────────────────────────────────────────────────┘
```

#### 核心数据结构

- **`pool_`**：`uint8_t*`，使用 `malloc` 分配的 1GB 连续内存空间
- **`meta_`**：`BlockMeta*`，使用 `malloc` 分配的 262,144 个块的元数据（使用 placement new 初始化，避免栈溢出）
- **`used_map`**：`std::bitset<kBlockCount>`，使用状态位图
- **`memory_info`**：`std::map<std::string, std::pair<size_t, size_t>>`，内存块映射表（key 为 Memory ID）
- **`memory_last_modified_time`**：`std::map<std::string, time_t>`，记录每个 Memory ID 最后修改内存的时间戳

### 3.2 核心算法

#### Memory ID 生成算法（Base62 编码）

**设计目标**：
- O(1) 时间复杂度生成 ID
- 超大容量（支持数亿到数百亿个ID）
- 向后兼容旧格式

**Base62 编码原理**：
- 字符集：`0-9`, `a-z`, `A-Z`（共 62 个字符）
- 5位Base62 = 916,132,832 个ID（约9亿）
- 6位Base62 = 56,800,235,584 个ID（约568亿）

**实现**：
- 使用 `uint64_t` 计数器递增生成数字
- Base62 编码将数字转换为字符串
- 自动扩展：超过5位最大值时自动扩展到6位
- 向后兼容：加载文件时自动识别旧格式（纯数字）和新格式（Base62）

**ID 格式示例**：
- `memory_00001`（第1个）
- `memory_001C8`（第100个）
- `memory_4c92`（第100万个）
- `memory_zzzzz`（5位最大值，第916,132,832个）
- `memory_000001`（自动扩展到6位）

**性能**：
- 生成 ID：O(1)
- 编码：O(log₆₂(n)) ≈ O(1)
- 初始化：O(m)，m 为已存在的内存块数（仅在加载时调用一次）

**关键函数**：
- `EncodeBase62(uint64_t num, size_t minLength)`：数字 → Base62 字符串
- `DecodeBase62(const std::string& str)`：Base62 字符串 → 数字
- `GenerateNextMemoryId()`：生成下一个 ID（O(1)）
- `InitializeMemoryIdCounter()`：初始化计数器（加载时调用）

#### 内存分配算法

**流程**：
1. 计算所需块数：`ceil(dataSize / 4KB)`
2. 检查总空间是否足够
3. 查找连续空闲块（首次适配）
4. 如果未找到但总空间足够，执行紧凑后重新查找
5. 写入数据并更新元数据和映射表

**首次适配算法**：
```cpp
int FindContinuousFreeBlock(blockCount) {
    for (i = 0; i < kBlockCount; i++) {
        if (used_map[i]) continue
        // 检查从 i 开始的连续空闲块
        j = i
        while (j < kBlockCount && !used_map[j]) j++
        if (j - i >= blockCount) return i
        i = j - 1  // 跳过已检查的块
    }
    return -1
}
```

**时间复杂度**：O(n)，n 为块数

#### 内存紧凑算法

**流程**：
1. 按 `memory_id` 为单位处理，而不是按块处理
2. 将 `memory_info` 按原始起始位置排序
3. 遍历每个 `memory_id`，一次性移动其所有块到 `freePos` 开始的位置
4. 更新 `memory_info` 中的起始位置
5. `freePos` 增加该 `memory_id` 的块数，继续处理下一个

**关键优化**：
- 按 `memory_id` 为单位整体移动，确保同一内存的所有块连续移动
- 按原始位置排序处理，确保移动顺序正确
- 使用 `std::set` 记录已处理的 `memory_id`，避免重复处理
- 确保 compact 后所有已使用的块连续排列，不留空隙

**时间复杂度**：O(n + m log m)，n 为块数，m 为不同 memory_id 的数量（排序开销）
**空间复杂度**：O(m)，m 为不同 memory_id 的数量

#### 位图优化

- 使用 `std::bitset<256>` 存储使用状态
- 查询/设置：O(1)
- 内存占用：32 字节
- 维护 `free_block_count` 变量，O(1) 时间判断是否有足够空间

---

## 4. 开发指南

### 4.1 代码规范

#### 命名规范
- **类名**：PascalCase，如 `SharedMemoryPool`
- **函数名**：PascalCase，如 `AllocateBlock()`
- **变量名**：snake_case，如 `free_block_count`
- **常量名**：k + PascalCase，如 `kPoolSize`
- **私有成员**：snake_case + 下划线后缀，如 `pool_`

#### 代码格式
- 使用 `clang-format` 格式化代码
- K&R 风格（左大括号不换行）
- 4 空格缩进

### 4.2 扩展开发

#### 添加新命令
1. 在 `commands.cpp` 的 `kCmds` 中添加命令定义
2. 在 `HandleCommand()` 中添加处理逻辑
3. 更新帮助信息

#### 修改内存池大小 ✅（已完成扩展至 1GB）

**当前状态**：内存池已扩展为 1GB（262,144 × 4KB 块）

**已完成的扩展**：
- ✅ 将内存池扩展到 1GB（262,144 × 4KB 块）
- ✅ 修复栈溢出问题：将 `std::vector` 改为使用 `malloc` 分配（更底层可控）
- ✅ 在 `Init()` 方法中使用 `malloc` 分配内存池和元信息数组
- ✅ 使用 placement new 初始化 `BlockMeta` 对象（因为包含 `std::string`）
- ✅ 更新 `kBlockCount` 的计算（自动计算）

**当前配置**：
```cpp
static constexpr size_t kPoolSize = 1024 * 1024 * 1024;  // 1GB
static constexpr size_t kBlockSize = 4096;               // 4KB
static constexpr size_t kBlockCount = kPoolSize / kBlockSize;  // 262,144 块
```

**技术要点**：
- 使用 `malloc`/`free` 替代 `std::vector` 分配内存（更底层可控，避免栈溢出）
- 在 `Init()` 方法中动态分配，确保内存分配在堆上
- 使用 placement new 初始化 `BlockMeta` 对象，确保 `std::string` 成员正确构造
- 在析构函数中正确调用 `BlockMeta` 的析构函数，确保 `std::string` 成员正确析构
- 持久化格式已支持更大的内存池（版本3）

**计划优化**：
- ⏳ 优化大内存池的分配和释放性能
- ⏳ 进一步优化内存紧凑算法

#### 添加持久化字段
1. 在 `FileHeader` 中添加字段
2. 更新文件版本号
3. 在 `Save()` 和 `Load()` 中处理新字段
4. 处理版本兼容性

#### 文件上传功能实现

**功能概述**：`alloc` 命令支持从文件读取内容并上传到内存池。

**实现位置**：`server/command/commands.cpp`

**核心函数**：
- `ReadFileContent(const std::string& filepath, std::string& content)`：读取文件内容
  - 检查文件扩展名是否为 `.txt`
  - 以二进制模式打开文件
  - 检测并移除 UTF-8 BOM（如果存在）
  - 读取文件内容到字符串
- `IsFilePath(const std::string& str)`：检测字符串是否为文件路径
  - 检查是否以 `@` 开头
  - 检查是否以 `.txt` 结尾
- `ExtractFilePath(const std::string& str)`：提取文件路径
  - 去掉 `@` 前缀
  - 处理带引号的路径

**使用方法**：
```bash
# 相对路径
alloc "文档" @file.txt
alloc "文档" @"../sample/test_file.txt"

# 绝对路径
alloc "文档" @C:\Users\file.txt
alloc "文档" @"C:\Users\My Documents\file.txt"
```

**注意事项**：
- 仅支持 `.txt` 文件格式
- 文件大小不能超过内存池限制（预留 1MB）
- 支持 UTF-8 编码，自动处理 BOM
- 路径可以包含空格，需要使用引号包裹

#### UTF-8 中文支持

**功能概述**：完整支持 UTF-8 编码的中文显示和文件读取。

**实现位置**：
- `server/main.cpp`：设置控制台代码页为 UTF-8
- `server/command/commands.cpp`：表格显示宽度计算和文件读取

**核心实现**：

1. **控制台 UTF-8 支持**（`main.cpp`）：
```cpp
SetConsoleOutputCP(65001); // UTF-8 code page
SetConsoleCP(65001);        // UTF-8 code page
```

2. **显示宽度计算**（`commands.cpp`）：
- `GetDisplayWidth(const std::string& str)`：计算字符串显示宽度
  - ASCII 字符：宽度为 1
  - UTF-8 中文字符（2-4 字节）：宽度为 2
- `PadToDisplayWidth(const std::string& str, size_t targetWidth)`：填充到指定显示宽度
- `TruncateToDisplayWidth(const std::string& str, size_t targetWidth)`：截断到指定显示宽度

3. **文件读取 UTF-8 支持**：
- 自动检测和移除 UTF-8 BOM（EF BB BF）
- 支持 UTF-8 编码的中文内容读取

**应用场景**：
- 表格列对齐（`status` 命令）
- 文件内容显示（`read` 命令）
- 文件上传（`alloc` 命令）

---

## 附录

### 性能指标

| 操作        | 时间复杂度 | 空间复杂度 |
| ----------- | ---------- | ---------- |
| 分配        | O(n)       | O(1)       |
| 释放        | O(1)       | O(1)       |
| 紧凑        | O(n)       | O(1)       |
| 查询        | O(1)       | O(1)       |
| **ID 生成** | **O(1)**   | O(1)       |

**空间占用**：
- 内存池：1GB（262,144 × 4KB 块）
- 元数据：~13-26 MB（取决于描述长度，动态分配在堆上）
- 位图：~32 KB（262,144 位）
- 内存映射表：O(m)，m 为内存块数
- 总计：约 1.03 GB（取决于元数据大小）

### 常见问题

**Q: 内存池大小是多少？**  
A: 当前配置为 1GB（262,144 × 4KB 块）。可通过修改 `core/shared_memory_pool/shared_memory_pool.h` 中的 `kPoolSize` 常量调整大小。已修复栈溢出问题，支持更大的内存池。

**Q: Memory ID 会用完吗？**  
A: 不会。系统使用 Base62 编码，5位支持约9亿个ID，6位支持约568亿个ID。即使每天分配100万个ID，也需要约50,000年才能用完6位Base62的容量。系统会自动扩展位数。

**Q: Memory ID 格式是什么？**  
A: 格式为 `memory_xxxxx`，其中 `xxxxx` 是 Base62 编码（0-9, a-z, A-Z）。例如：`memory_00001`（第1个）、`memory_001C8`（第100个）、`memory_4c92`（第100万个）。系统向后兼容旧的纯数字格式。

**Q: 持久化文件损坏怎么办？**  
A: 程序会自动检测文件格式，如果损坏会使用新内存池。可以删除损坏的文件重新开始。

**Q: 如何实现多客户端并发访问？**  
A: 需要为 `SharedMemoryPool` 的操作添加互斥锁（`std::mutex`），确保线程安全。当前系统使用 Memory ID 标识内存块，所有客户端共享访问。

---

**文档版本**：v1.0  
**最后更新**：2026年

**未来开发计划**：
1. ✅ **内存池扩展**：已完成，当前支持 1GB 内存池（已修复栈溢出问题）
2. ⏳ **紧凑算法优化**：改进内存紧凑算法，减少拷贝次数，提高效率
3. ⏳ **SDK/DLL 打包**：将核心功能打包为 DLL，提供 C/C++ SDK 接口
4. ⚠️ **TCP 连接修复**：修复 TCP 连接功能存在的问题
5. ⏳ **客户端文件上传**：支持客户端通过 TCP 上传文件到内存池（服务器端文件上传功能已完成）
6. ⏳ **性能优化**：优化大内存池的分配和释放性能
