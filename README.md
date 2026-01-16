# Shared Memory Management System

![python](https://img.shields.io/badge/python-3.10%20%7C%203.11%20%7C%203.12-blue?logo=python&logoColor=white)
![platform](https://img.shields.io/badge/platform-windows-lightgrey?logo=windows&logoColor=white)
![license](https://img.shields.io/badge/license-MIT-green)

## 目录

- [Shared Memory Management System](#shared-memory-management-system)
  - [目录](#目录)
  - [项目简介](#项目简介)
  - [核心特性](#核心特性)
    - [内存管理](#内存管理)
    - [已实现功能](#已实现功能)
      - [1. 内存池初始化与重置](#1-内存池初始化与重置)
      - [2. 内存增删查改（CRUD）](#2-内存增删查改crud)
      - [3. 内存紧凑（`compact`）](#3-内存紧凑compact)
      - [4. 状态查询（`status` / `info`）](#4-状态查询status--info)
      - [5. 命令行界面](#5-命令行界面)
      - [6. 数据持久化](#6-数据持久化)
      - [7. 批量执行（`exec`）](#7-批量执行exec)
      - [8. TCP 客户端接口 ✅](#8-tcp-客户端接口-)
      - [9. 文件上传功能 ✅](#9-文件上传功能-)
      - [10. UTF-8 中文支持 ✅](#10-utf-8-中文支持-)
  - [技术架构](#技术架构)
    - [核心类设计](#核心类设计)
      - [`SharedMemoryPool`](#sharedmemorypool)
      - [关键数据结构](#关键数据结构)
    - [内存布局](#内存布局)
    - [数据持久化文件格式](#数据持久化文件格式)
      - [文件结构（按顺序）](#文件结构按顺序)
      - [文件格式说明](#文件格式说明)
      - [文件大小估算](#文件大小估算)
  - [使用方法](#使用方法)
    - [编译与运行](#编译与运行)
      - [方式一：使用批处理脚本（推荐）](#方式一使用批处理脚本推荐)
      - [方式二：手动编译](#方式二手动编译)
    - [命令示例](#命令示例)
    - [状态输出示例](#状态输出示例)
  - [外机访问快速开始](#外机访问快速开始)
    - [快速开始步骤](#快速开始步骤)
      - [步骤 1：启动服务器（本地）](#步骤-1启动服务器本地)
      - [步骤 2：配置防火墙（服务器端）](#步骤-2配置防火墙服务器端)
      - [步骤 3：连接客户端（外机）](#步骤-3连接客户端外机)
    - [文档阅读顺序](#文档阅读顺序)
      - [最小化阅读（快速上手）](#最小化阅读快速上手)
      - [完整阅读（深入了解）](#完整阅读深入了解)
    - [客户端使用方式](#客户端使用方式)
      - [方式 A：使用 Python 客户端（推荐，最简单）](#方式-a使用-python-客户端推荐最简单)
      - [方式 B：使用 C++ SDK（命令行工具，推荐用于命令行操作）](#方式-b使用-c-sdk命令行工具推荐用于命令行操作)
      - [方式 C：使用 DLL/SDK（本地调用，无需网络）](#方式-c使用-dllsdk本地调用无需网络)
    - [重要提示](#重要提示)
    - [故障排除](#故障排除)
  - [网络连接故障排除](#网络连接故障排除)
    - [问题分析：多个 IP 地址](#问题分析多个-ip-地址)
      - [为什么有多个 IP 地址？](#为什么有多个-ip-地址)
      - [为什么只有主网卡 IP 能 ping 通？](#为什么只有主网卡-ip-能-ping-通)
      - [应该使用哪个 IP 连接？](#应该使用哪个-ip-连接)
      - [ping 不通，TCP 能连接吗？](#ping-不通tcp-能连接吗)
    - [解决方案](#解决方案)
      - [步骤 1：确认使用正确的 IP](#步骤-1确认使用正确的-ip)
      - [步骤 2：测试 TCP 连接（推荐）](#步骤-2测试-tcp-连接推荐)
      - [步骤 3：配置防火墙（如果 TCP 连接失败）](#步骤-3配置防火墙如果-tcp-连接失败)
      - [步骤 4：验证服务器监听状态](#步骤-4验证服务器监听状态)
    - [完整测试流程](#完整测试流程)
      - [在服务器上](#在服务器上)
      - [在外机上](#在外机上)
    - [常见问题](#常见问题)
      - [Q1: 为什么服务器显示多个 IP？](#q1-为什么服务器显示多个-ip)
      - [Q2: 为什么虚拟网卡 IP ping 不通？](#q2-为什么虚拟网卡-ip-ping-不通)
      - [Q3: 我应该禁用其他网卡吗？](#q3-我应该禁用其他网卡吗)
      - [Q4: 如何确认哪个 IP 是主网卡？](#q4-如何确认哪个-ip-是主网卡)
      - [Q5: ping 通了，但 TCP 连接失败？](#q5-ping-通了但-tcp-连接失败)
  - [项目结构](#项目结构)
  - [技术栈](#技术栈)
  - [代码质量](#代码质量)
  - [开发进度](#开发进度)
  - [待实现功能](#待实现功能)
    - [短期目标](#短期目标)
    - [长期目标](#长期目标)
      - [1. 内存池扩展 ✅（已完成）](#1-内存池扩展-已完成)
      - [2. 内存紧凑算法优化 ✅（已完成）](#2-内存紧凑算法优化-已完成)
      - [3. SDK/DLL 打包（部分完成）](#3-sdkdll-打包部分完成)
      - [4. 文件上传功能 ✅（已完成）](#4-文件上传功能-已完成)
      - [其他计划](#其他计划)
  - [性能指标](#性能指标)
  - [注意事项](#注意事项)
  - [更新日志](#更新日志)
    - [v0.7.0 (当前版本)](#v070-当前版本)
    - [v0.6.0](#v060)
    - [v0.5.0](#v050)
    - [v0.4.0](#v040)
    - [v0.3.2](#v032)
    - [v0.3.1](#v031)
    - [v0.3.0](#v030)
    - [v0.2.0](#v020)
    - [v0.1.0](#v010)
  - [客户端实现](#客户端实现)
    - [快速开始](#快速开始)
    - [协议规范](#协议规范)
    - [客户端实现](#客户端实现-1)

---

## 项目简介

本项目是一个基于 C++17 实现的共享内存管理系统，采用固定块大小（4KB）的内存池管理策略，提供高效的内存分配、释放和紧凑功能。系统通过 TCP 服务器提供客户端接口，支持多客户端通过网络连接访问共享内存池。客户端可以使用任何支持 TCP Socket 的编程语言实现。

## 核心特性

### 内存管理
- **固定块大小管理**：将内存池划分为 262,144 个 4KB 的固定大小块（当前配置为 1GB 内存池）
- **连续块分配**：支持跨多个块的数据存储，自动计算所需块数
- **内存紧凑（Compaction）**：当找不到连续空闲块但总空间足够时，自动进行内存紧凑，合并碎片
- **Memory ID 标识**：使用 Base62 编码的 ID 系统（`memory_00001`, `memory_001C8`, `memory_4c92` 等）
  - **O(1) 生成**：使用计数器直接生成，无需遍历
  - **超大容量**：5位支持约9亿个ID，6位支持约568亿个ID，7位支持约3521亿个ID（自动扩展）
- **内容描述**：每个内存块都有描述信息，方便识别和管理

### 已实现功能

#### 1. 内存池初始化与重置
- 自动分配 1GB 连续内存空间（262,144 个 4KB 块）
- 支持内存池重置（`reset`）：清空所有内存数据并将元数据重置为默认状态
  - 需要密码确认，防止误操作
  - 重置后内存池恢复到初始状态（所有块为空闲）

#### 2. 内存增删查改（CRUD）
- **分配（`alloc`）**：
  - `alloc "<description>" "<content>"`：分配内存并指定描述信息
  - `alloc "<description>" @<filepath>`：从文件上传内容到内存池（支持 .txt 文件）
    - 支持相对路径和绝对路径
    - 支持带引号的路径：`@"path/to/file.txt"` 或 `@"C:\Users\file.txt"`
    - 自动检测 UTF-8 BOM 并移除
  - 自动生成 Memory ID（Base62 编码，O(1) 生成，格式：`memory_00001`, `memory_001C8`, `memory_4c92` 等）
  - 自动计算所需块数（向上取整）
  - 空间不足时自动触发紧凑操作
  - 返回分配的 Memory ID 和起始块 ID
- **释放（`free` / `delete`）**：
  - `free <memory_id>`：释放指定 Memory ID 的所有内存块
  - `delete <memory_id>`：释放指定 Memory ID 的所有内存块（`free` 的别名）
- **更新（`update`）**：
  - `update <memory_id> "<new_content>"`：更新指定 Memory ID 的内存内容
  - 如果新内容大小超过原分配，自动重新分配
  - 如果新内容大小不超过原分配，直接覆盖写入
  - 保持原有的 Memory ID 和描述信息
- **读取（`read`）**：
  - `read <memory_id>`：显示指定 Memory ID 的内容
  - 直接从内存池读取，遇到 0 停止
  - 输出格式：元信息 + 虚线分隔 + 内容（原样输出，不添加引号）+ 虚线分隔 + 大小和修改时间
  - 使用虚线分隔元信息和内容，内容原样输出（支持多行和中文）

#### 3. 内存紧凑（`compact`）
- 自动合并碎片，将已使用的块移动到内存池前端
- 按 `memory_id` 为单位整体移动，确保同一内存的所有块连续移动
- 更新内存块信息映射关系，确保 compact 后所有已使用的块连续排列

#### 4. 状态查询（`status` / `info`）<a id="4-状态查询status & info"></a>
- `status --memory`：显示内存池使用情况，按 Memory ID 展示占用范围（格式：`block_000 - block_015(16 blocks, 64KB)`）
  - 包含列：Memory ID、Description、Bytes（实际数据大小）、Range（包含块数和大小信息）、Last Modified
  - 支持中文显示，表格列自动对齐（中文字符按 2 个显示宽度计算）
- `status --block`：只显示已使用的块，包括 Memory ID、描述和最后修改时间（格式：`block_000`, `block_001` 等）
  - 支持中文显示，表格列自动对齐
- `info`：显示系统综合信息
  - 内存池配置（总大小、块大小、总块数）
  - 使用情况统计（已用/空闲块数、使用率、已用/空闲字节数）
  - 内存分布信息（最大连续空闲块、碎片数量、是否需要紧凑操作）
  - 内存统计（活跃内存数量、平均块数、最大/最小使用）
  - 持久化状态

#### 5. 命令行界面
- 交互式 REPL（Read-Eval-Print Loop）界面
- 完整的帮助系统（`help` 命令）
- 优雅的退出机制（`quit` / `exit`）

#### 6. 数据持久化
- 自动保存内存池状态到 `memory_pool.dat` 文件
- 支持程序正常退出（`quit`/`exit`）时保存
- 支持异常退出（Ctrl+C、Ctrl+Z）时自动保存
- 程序启动时自动加载之前保存的状态
- 二进制文件格式，包含文件头、元数据、使用状态位图、内存块信息、最后修改时间、Next Fit 搜索位置和内存池数据
- 支持向后兼容：如果加载的文件缺少 `next_search_pos_`，会自动计算第一个空闲位置

#### 7. 批量执行（`exec`）
- `exec <filename>`：从文件读取并执行命令
- 支持空行和注释（以 `#` 开头）
- 显示执行的行号和命令
- 遇到 `quit`/`exit` 时停止执行（不退出程序）

#### 8. TCP 客户端接口 ✅
- 服务器启动时自动监听端口（默认 8888）
- **支持外部主机连接**：服务器绑定到 `0.0.0.0`，可接受来自局域网和公网的连接
- 支持多客户端并发连接
- 提供完整的协议规范和客户端实现指南
- 支持所有内存管理操作（ALLOC、READ、UPDATE、DELETE、STATUS）
- 使用 Memory ID 系统，所有客户端共享访问
- **配置要求**：需要配置防火墙允许端口 8888，公网访问需要配置路由器端口转发
- 详细配置指南：参考 `server/network/EXTERNAL_ACCESS.md`

#### 9. 文件上传功能 ✅
- `alloc` 命令支持从文件上传内容
- 支持 `.txt` 文件格式
- 支持相对路径和绝对路径
- 自动检测和移除 UTF-8 BOM
- 文件大小检查，确保不超过内存池限制

#### 10. UTF-8 中文支持 ✅
- 控制台代码页设置为 UTF-8（65001）
- 文件读取支持 UTF-8 编码
- 表格显示支持中文对齐（中文字符按 2 个显示宽度计算）
- 内容显示支持中文和多字节字符

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
│  Memory Pool (1GB = 262,144 × 4KB blocks)  │
├─────────────────────────────────────────┤
│ Block 0   │ Block 1   │ ... │ Block 262143 │
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
│    - reserved_version: uint32_t (保留字段，不再使用)     │
│    - free_block_count: size_t (空闲块数量)                │
│    - memory_info_count: size_t (内存信息条目数)           │
│    - reserved: uint64_t[4] (预留字段)                    │
├─────────────────────────────────────────────────────────┤
│ 2. BlockMeta Array (元数据数组，262,144个块)            │
│    对每个块 i (0-262143):                                 │
│    - used: bool (是否被使用)                              │
│    - memoryIdLen: size_t (Memory ID 长度)                │
│    - memory_id: char[memoryIdLen] (Memory ID 字符串)     │
│    - descLen: size_t (描述长度)                          │
│    - description: char[descLen] (描述字符串)             │
├─────────────────────────────────────────────────────────┤
│ 3. Used Map (使用状态位图)                                │
│    - bitsetBytes: uint8_t[(262144+7)/8] (32KB)          │
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
│ 6. Next Search Position (Next Fit 优化位置)             │
│    - next_search_pos_: size_t (下次分配时的搜索起始位置) │
├─────────────────────────────────────────────────────────┤
│ 7. Pool Data (内存池数据)                                 │
│    - pool_: uint8_t[kPoolSize] (内存池原始数据)          │
└─────────────────────────────────────────────────────────┘
```

#### 文件格式说明

- **文件类型**：二进制文件
- **字节序**：小端序（Little-Endian，Windows 默认）
- **魔数**：`0x4D454D50`（ASCII: "MEMP"），用于文件格式验证
- **版本号**：已移除版本字段，使用 `reserved_version` 占位（不再使用）
- **向后兼容**：支持加载旧格式文件（如果缺少 `next_search_pos_`，会自动计算第一个空闲位置）

#### 文件大小估算

- 文件头：`sizeof(FileHeader)` ≈ 48 字节
- 元数据数组：262,144 × (1 + 8 + 8 + 可变字符串长度) ≈ 4-15 MB（取决于字符串长度）
- Used Map：32,768 字节（32KB）
- Memory Info：可变（取决于内存块数量）
- Memory Last Modified Time：可变（取决于内存块数量）
- Next Search Position：`sizeof(size_t)` ≈ 8 字节
- Pool Data：kPoolSize 字节（当前为 1GB）
- **总大小**：约 1GB+（取决于元数据大小）

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
g++ -std=c++17 -Wall main.cpp command/commands.cpp ../core/shared_memory_pool/shared_memory_pool.cpp ../core/persistence/persistence.cpp network/protocol.cpp network/tcp_server.cpp -o main.exe -lws2_32
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
| MemoryID     | Description | Bytes | range                                  | Last Modified       |
| ------------ | ----------- | ----- | -------------------------------------- | ------------------- |
| memory_00001 | User Data   | 13    | block_000 - block_015(16 blocks, 64KB) | 2026-01-15 10:30:45 |
| memory_00002 | Config Data | 256   | block_016 - block_031(16 blocks, 64KB) | 2026-01-15 11:20:10 |
```

**`status --block` 输出：**
```
Block Pool Status:
| ID        | MemoryID     | Description | Last Modified       |
| --------- | ------------ | ----------- | ------------------- |
| block_000 | memory_00001 | User Data   | 2026-01-15 10:30:45 |
| block_001 | memory_00001 | User Data   | 2026-01-15 10:30:45 |
```

**`read <memory_id>` 输出：**
```
Memory ID: memory_00001
Description: User Data
Blocks: 0-0
----------------------------------------
Hello World
----------------------------------------
Size: 11 bytes
Last Modified: 2026-01-15 10:30:45
```

## 外机访问快速开始

如果你想在外机访问 server，可以按照以下步骤操作：

### 快速开始步骤

#### 步骤 1：启动服务器（本地）

```bash
cd server
.\run.bat
```

记录服务器显示的 IP 地址，例如：`192.168.1.100:8888`

#### 步骤 2：配置防火墙（服务器端）

```powershell
# 以管理员身份运行 PowerShell
netsh advfirewall firewall add rule name="SMM Server" dir=in action=allow protocol=TCP localport=8888
```

#### 步骤 3：连接客户端（外机）

**Python 方式**：
```python
from client.client import MemoryClient

client = MemoryClient("192.168.1.100", 8888)  # 使用服务器 IP
client.connect()
memory_id = client.alloc("测试", "Hello World")
print(client.read(memory_id))
```

**C++ CLI 方式**：
```bash
# 在外机编译并运行
cd sdk
.\build.bat
.\examples\client_cli.exe 192.168.1.100 8888
```

### 文档阅读顺序

#### 最小化阅读（快速上手）

1. **`README.md`** - 查看"编译与运行"章节，启动服务器
2. **`server/network/README_TCP.md`** - 查看"外部主机访问配置"章节，配置防火墙
3. **`client/README.md`** - 查看"快速开始"和示例代码，使用 Python 客户端

#### 完整阅读（深入了解）

1. **`README.md`** - 了解项目整体和服务器使用
2. **`server/network/README_TCP.md`** - 了解协议规范、外部访问配置、完整连接流程
3. **`client/README.md`** 或 **`sdk/README.md`** - 根据选择的客户端方式阅读

### 客户端使用方式

#### 方式 A：使用 Python 客户端（推荐，最简单）

**查看文档**：`client/README.md`

**快速使用**：
```python
from client.client import MemoryClient

# 连接到服务器（使用服务器 IP，不是 localhost）
client = MemoryClient("192.168.1.100", 8888)  # 使用服务器 IP
client.connect()

# 分配内存
memory_id = client.alloc("描述", "内容")

# 读取内存
content = client.read(memory_id)

# 释放内存
client.delete(memory_id)
```

#### 方式 B：使用 C++ SDK（命令行工具，推荐用于命令行操作）

**适用场景**：想要像 server 端一样通过命令行交互，不需要写代码。

**查看文档**：`sdk/README.md`

**快速开始**：

```bash
# 1. 编译客户端 CLI 工具
cd sdk
.\build.bat

# 2. 运行客户端（交互式命令行，与 server 端使用方式完全相同）
.\examples\client_cli.exe 192.168.1.100 8888
```

**使用示例**（与 server 端命令完全相同）：
```
##############################################
#                                            #
#        Shared Memory Manager Client        #
#                                            #
##############################################

Connected to server at 192.168.1.100:8888
Type 'help' for help, 'quit' or 'exit' to exit.

client> alloc "测试数据" "Hello World"
Memory allocated: memory_00001

client> read memory_00001
----------------------------------------
Memory ID: memory_00001
Description: 测试数据
Last Modified: 2026-01-15 10:30:45
----------------------------------------
Hello World

client> status --memory
Memory Pool Status:
  Total Blocks: 262144
  Used Blocks: 1
  Free Blocks: 262143
  ...

client> quit
Bye!
```

**支持的命令**（与 server 端完全相同）：
- `alloc "<description>" "<content>"` - 分配内存
- `read <memory_id>` - 读取内存内容
- `update <memory_id> "<new_content>"` - 更新内存内容
- `free <memory_id>` - 释放内存
- `delete <memory_id>` - 删除内存（free 的别名）
- `status [--memory|--block]` - 查询状态
- `info` - 显示系统信息
- `help` - 显示帮助信息
- `quit` / `exit` - 退出客户端

**命令行参数**：
```bash
# 使用默认地址和端口（127.0.0.1:8888）
.\examples\client_cli.exe

# 指定服务器地址
.\examples\client_cli.exe 192.168.1.100

# 指定服务器地址和端口
.\examples\client_cli.exe 192.168.1.100 8888
```

#### 方式 C：使用 DLL/SDK（本地调用，无需网络）

**查看文档**：`sdk/README.md`（DLL/SDK 方式章节）

**说明**：这种方式是直接链接 DLL，不需要网络连接，但需要将 DLL 部署到客户端机器。

**适用场景**：客户端和服务器在同一台机器，或需要本地高性能调用。

### 重要提示

1. **服务器 IP 地址**：客户端必须使用服务器显示的 IP 地址（如 `192.168.1.100`），**不能使用 `localhost` 或 `127.0.0.1`**
2. **防火墙配置**：**必须配置**，否则外部主机无法连接
3. **公网访问**：需要配置路由器端口转发（见 `server/network/README_TCP.md` 的"路由器配置"章节）
4. **端口**：默认端口是 8888，确保没有被其他程序占用

### 故障排除

如果遇到连接问题，按顺序检查：

1. **服务器是否启动**：查看服务器控制台输出
2. **防火墙是否配置**：`netstat -an | findstr :8888` 应该显示 `0.0.0.0:8888`
3. **IP 地址是否正确**：使用服务器显示的 IP，不是 localhost
4. **网络是否连通**：使用 `ping` 测试网络连通性

详细故障排除请参考：[网络连接故障排除](#网络连接故障排除)章节。

---

## 网络连接故障排除

### 问题分析：多个 IP 地址

#### 为什么有多个 IP 地址？

你的电脑有多个网络接口（网卡），每个接口都有自己的 IP 地址：

1. **主网卡 IP**（如 `192.168.20.31`）- 主网卡（有线/无线网卡），连接到路由器/交换机
2. **虚拟网卡 IP**（如 `172.29.56.108`）- 可能是：
   - 虚拟网卡（VMware、VirtualBox、WSL、Docker 等）
   - VPN 虚拟网卡
   - Hyper-V 虚拟交换机
   - 其他虚拟网络适配器

#### 为什么只有主网卡 IP 能 ping 通？

**原因分析**：

1. **网段不同**：
   - 主网卡 IP 和外机在**同一个局域网**（如 `192.168.20.x`）
   - 虚拟网卡 IP 是**不同的网段**（可能是虚拟网络）
   - 外机无法直接访问不同网段的 IP

2. **路由问题**：
   - 外机只能访问同一局域网内的 IP
   - 虚拟网卡的 IP 通常只在虚拟网络内有效，外机无法访问

3. **防火墙/网络隔离**：
   - 某些虚拟网卡可能被防火墙隔离
   - 虚拟网络可能配置为不对外暴露

#### 应该使用哪个 IP 连接？

**✅ 使用主网卡 IP（外机可以 ping 通的 IP）**

**原因**：
- ✅ 外机可以 ping 通，说明网络层是通的
- ✅ 这是主网卡的 IP，连接到实际的局域网
- ✅ 服务器绑定到 `0.0.0.0`（所有接口），所以通过这个 IP 可以访问

#### ping 不通，TCP 能连接吗？

**一般情况**：
- 如果 **ping 不通**，**TCP 通常也连不上**
- ping 使用 ICMP 协议，TCP 使用 TCP 协议
- 如果网络层不通（ping 不通），传输层（TCP）也无法建立连接

**特殊情况**（很少见）：
- 防火墙可能只阻止 ICMP，但允许 TCP
- 但如果是网络层不通（不同网段），所以 TCP 也连不上

**结论**：**使用外机可以 ping 通的主网卡 IP 连接**

### 解决方案

#### 步骤 1：确认使用正确的 IP

**客户端连接时使用**：
```
主网卡 IP:8888（外机可以 ping 通的 IP）
```

**不要使用**：
- ❌ 虚拟网卡 IP - ping 不通，无法连接

#### 步骤 2：测试 TCP 连接（推荐）

ping 只能测试 ICMP，不能测试 TCP。应该测试 TCP 连接：

**在外机上测试**：

```bash
# 方法 1：使用 telnet（Windows）
telnet 192.168.20.31 8888

# 方法 2：使用 PowerShell（Windows）
Test-NetConnection -ComputerName 192.168.20.31 -Port 8888

# 方法 3：使用 nc（Linux/Mac）
nc -zv 192.168.20.31 8888

# 方法 4：使用 Python 快速测试
python -c "import socket; s = socket.socket(); s.settimeout(3); result = s.connect_ex(('192.168.20.31', 8888)); print('Connected!' if result == 0 else 'Failed'); s.close()"
```

**预期结果**：
- ✅ 如果连接成功，说明 TCP 端口是开放的
- ❌ 如果连接失败，可能是防火墙阻止了 TCP 8888 端口

#### 步骤 3：配置防火墙（如果 TCP 连接失败）

**在服务器上（以管理员身份运行）**：

```powershell
# 允许 TCP 8888 端口入站连接
netsh advfirewall firewall add rule name="SMM Server" dir=in action=allow protocol=TCP localport=8888

# 验证规则
netsh advfirewall firewall show rule name="SMM Server"
```

#### 步骤 4：验证服务器监听状态

**在服务器上运行**：

```powershell
netstat -an | findstr :8888
```

**应该看到**：
```
TCP    0.0.0.0:8888           0.0.0.0:0              LISTENING
```

**说明**：
- `0.0.0.0:8888` 表示服务器监听所有网络接口的 8888 端口
- 这意味着可以通过任何 IP（包括主网卡 IP）访问

### 完整测试流程

#### 在服务器上

1. **启动服务器**：
   ```bash
   cd server
   .\run.bat
   ```

2. **记录可用的 IP**：
   - 使用外机可以 ping 通的主网卡 IP

3. **配置防火墙**：
   ```powershell
   netsh advfirewall firewall add rule name="SMM Server" dir=in action=allow protocol=TCP localport=8888
   ```

4. **验证监听**：
   ```powershell
   netstat -an | findstr :8888
   ```

#### 在外机上

1. **测试网络连通性**：
   ```bash
   ping 192.168.20.31
   # 应该成功
   ```

2. **测试 TCP 连接**：
   ```bash
   # Windows
   Test-NetConnection -ComputerName 192.168.20.31 -Port 8888
   
   # 或使用 telnet
   telnet 192.168.20.31 8888
   ```

3. **连接客户端**：
   ```python
   # Python 客户端
   from client.client import MemoryClient
   
   client = MemoryClient("192.168.20.31", 8888)  # 使用正确的 IP
   client.connect()
   ```

### 常见问题

#### Q1: 为什么服务器显示多个 IP？

**A**: 你的电脑有多个网络接口（网卡），每个接口都有自己的 IP。服务器会显示所有接口的 IP，但只有连接到实际局域网的 IP 才能被外机访问。

#### Q2: 为什么虚拟网卡 IP ping 不通？

**A**: 这个 IP 可能是虚拟网卡的 IP（如 WSL、Docker、VMware 等）。虚拟网卡的 IP 通常只在虚拟网络内有效，外机无法直接访问。

#### Q3: 我应该禁用其他网卡吗？

**A**: **不需要**。服务器绑定到 `0.0.0.0`，会监听所有接口。只要使用正确的 IP（主网卡 IP）连接即可。

#### Q4: 如何确认哪个 IP 是主网卡？

**A**: 
- 查看 `ipconfig` 输出，找到"默认网关"对应的网卡
- 或者测试哪个 IP 能被外机 ping 通

#### Q5: ping 通了，但 TCP 连接失败？

**A**: 
1. 检查防火墙是否允许 TCP 8888 端口
2. 确认服务器正在运行
3. 确认服务器监听在 `0.0.0.0:8888`（不是 `127.0.0.1:8888`）

---

## 项目结构

```
Shared-Memory-Manager/
├── core/                                 # 核心库（可编译成 DLL）
│   ├── shared_memory_pool/              # 内存池核心模块
│   │   ├── shared_memory_pool.h         # 内存池类声明
│   │   └── shared_memory_pool.cpp       # 内存池实现
│   ├── persistence/                      # 持久化模块
│   │   ├── persistence.h                # 持久化模块声明
│   │   └── persistence.cpp               # 持久化实现
│   ├── api/                              # C API 包装层
│   │   ├── smm_api.h                    # C API 头文件
│   │   └── smm_api.cpp                  # C API 实现
│   └── build_dll.bat                    # DLL 编译脚本
├── server/                               # 服务器端程序（使用 core/）
│   ├── main.cpp                         # 主程序入口，REPL 循环
│   ├── command/                         # 命令处理模块
│   │   ├── commands.h                   # 命令处理声明
│   │   └── commands.cpp                 # 命令处理实现
│   ├── network/                          # 网络模块（TCP 客户端接口）
│   │   ├── tcp_server.h                # TCP 服务器声明
│   │   ├── tcp_server.cpp              # TCP 服务器实现
│   │   ├── protocol.h                  # 网络协议定义
│   │   ├── protocol.cpp                 # 协议编解码实现
│   │   └── README_TCP.md               # TCP 客户端接口设计说明
│   ├── run.bat                          # 编译运行脚本
│   └── build.bat                        # 编译脚本
├── sdk/                                  # SDK 发布包结构
│   ├── include/                         # 头文件目录
│   │   └── client_sdk.h                # C++ 客户端 SDK 头文件
│   ├── src/                             # 源代码目录
│   │   └── client_sdk.cpp              # C++ 客户端 SDK 实现
│   ├── examples/                        # 示例代码目录
│   │   ├── basic_usage.cpp             # C API 基础使用示例
│   │   ├── client_cli.cpp              # C++ 客户端 CLI 示例
│   │   └── use_dll.cpp                 # DLL 直接使用示例
│   ├── lib/                             # 库文件目录
│   ├── docs/                            # 文档目录
│   ├── build.bat                        # SDK 编译脚本
│   └── README.md                        # SDK 说明
├── client/                               # 客户端实现
│   ├── client.py                        # 客户端参考实现
│   └── README.md                        # 客户端实现指南（适用于所有语言）
├── sample/                              # 示例文件目录
│   ├── env.txt                          # 6KB 文件示例（2 blocks）
│   ├── error.txt                        # 153KB 文件示例
│   └── 测试中文.txt                     # 中文测试文件
├── README.md                            # 项目说明文档（本文件）
├── doc.md                               # 技术文档（面向开发者）
├── DLL_SDK_GUIDE.md                     # DLL/SDK 打包指南
├── JAVA_GC_REFERENCE.md                 # Java GC 原理参考
├── example.txt                          # 示例文件
└── problem.txt                          # 问题记录
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
| 内存增删查改   | ✅ 完成   | 100%   |
| 内存紧凑       | ✅ 完成   | 100%   |
| 状态查询       | ✅ 完成   | 100%   |
| 命令行界面     | ✅ 完成   | 100%   |
| 批量执行       | ✅ 完成   | 100%   |
| 数据持久化     | ✅ 完成   | 100%   |
| TCP 服务器接口 | 🔄 进行中 | 60%    |
| 客户端接口规范 | 🔄 进行中 | 60%    |
| 文件上传功能   | ✅ 完成   | 100%   |
| UTF-8 中文支持 | ✅ 完成   | 100%   |
| 表格显示优化   | ✅ 完成   | 100%   |
| 内存池扩展(1G) | ✅ 完成   | 100%   |
| 紧凑算法优化   | ⏳ 计划中 | 0%     |
| SDK/DLL 打包   | 🔄 进行中 | 40%    |

## 待实现功能

### 短期目标
- [x] 完成数据持久化功能（保存/加载内存池状态）
- [x] 实现批量执行功能（`exec` 命令）
- [x] TCP 服务器功能（监听端口，提供客户端接口）
- [x] 网络协议实现（请求/响应格式，支持 Memory ID 系统）
- [x] 客户端接口规范和实现指南

### 长期目标

#### 1. 内存池扩展 ✅（已完成）
- [x] 将内存池从 100MB 扩展到 1GB
- [x] 修复栈溢出问题（使用 `malloc` 替代 `std::vector`，更底层可控）
- [x] 支持动态分配元信息数组（使用 `malloc` + placement new）
- [x] 优化内存分配方式，使用底层 `malloc`/`free` 管理
- [ ] 优化大内存池的分配和释放性能（计划中）

#### 2. 内存紧凑算法优化 ✅（已完成）
- [x] 优化紧凑算法，按 `memory_id` 为单位整体移动
- [x] 修复多块内存的移动问题，确保同一 `memory_id` 的所有块连续移动
- [x] 按原始位置排序处理，确保移动顺序正确
- [ ] 支持增量紧凑（部分紧凑）（计划中）

#### 3. SDK/DLL 打包（部分完成）
- [x] 目录结构重组（core/、sdk/）
- [x] C API 包装层实现（core/api/）
- [x] SDK 基础示例代码（sdk/examples/）
- [ ] 编译为 DLL
- [ ] 提供完整的 SDK 文档
- [ ] 多语言绑定（C#、Python、Java 等）

#### 4. 文件上传功能 ✅（已完成）
- [x] 支持服务器端通过 `alloc` 命令上传文件（`.txt` 格式）
- [x] 支持相对路径和绝对路径
- [x] 自动检测 UTF-8 BOM
- [ ] 支持客户端通过 TCP 上传文件（计划中）
- [ ] 实现文件分块上传（计划中）
- [ ] 支持更多文件格式（计划中）

#### 其他计划
- [ ] 日志系统
- [ ] 单元测试
- [ ] 性能监控和统计

## 性能指标

- **内存池大小**：1GB（262,144 × 4KB 块）
  - **已支持**：可通过修改 `core/shared_memory_pool/shared_memory_pool.h` 中的 `kPoolSize` 常量调整大小
- **分配算法**：Next Fit（下次适配）+ 自动紧凑
  - **Next Fit 优化**：使用 `next_search_pos_` 记录搜索起始位置，避免每次都从0开始搜索
  - **自动更新**：分配后更新为分配结束位置，compact 后更新为第一个空闲位置，free 后如果释放位置更靠前则更新
- **Memory ID 生成**：O(1) 时间复杂度（Base62 编码）
  - **容量**：5位支持约9亿个ID，6位支持约568亿个ID，7位支持约3521亿个ID（自动扩展）
  - **格式**：`memory_xxxxx`（xxxxx 是 Base62 编码：0-9, a-z, A-Z）
- **时间复杂度**：
  - 分配：O(n) 最坏情况，n 为块数（Next Fit 优化后平均情况更好）
  - 紧凑：O(n + m log m)，n 为块数，m 为不同 memory_id 的数量
  - 释放：O(1)
  - 查询：O(1) 到 O(n)
  - **ID 生成**：O(1)

## 注意事项

1. **客户端接口**：系统通过 TCP 服务器提供客户端接口，客户端通过网络连接访问共享内存池，无需单独的客户端程序
   - **注意**：TCP 连接功能目前存在一些问题，正在修复中
2. **Memory ID 系统**：使用 Base62 编码的 ID（`memory_00001`, `memory_001C8`, `memory_4c92` 等）区分内存块
   - O(1) 生成，超大容量（5位约9亿，6位约568亿，7位约3521亿），自动扩展
   - 所有客户端共享访问
3. **数据持久化**：功能已完成，支持程序退出时自动保存状态（Ctrl+C、Ctrl+Z、quit/exit）
4. **内存池大小**：可在 `core/shared_memory_pool/shared_memory_pool.h` 中通过 `kPoolSize` 常量调整（当前为 1GB，使用 `malloc` 分配，避免栈溢出）
5. **编译器要求**：建议使用支持 C++17 的编译器（g++ 7.0+ 或 MSVC 2017+）
6. **块 ID 格式**：显示格式为 3 位数字，不足 3 位用 0 填充（如 `block_000`, `block_030`）
7. **持久化文件**：`memory_pool.dat` 保存在服务器程序运行目录，不应提交到版本控制（已在 `.gitignore` 中配置）
8. **文件上传**：`alloc` 命令支持从文件上传内容，仅支持 `.txt` 文件格式，支持相对路径和绝对路径
9. **中文支持**：系统完整支持 UTF-8 编码的中文显示和文件读取，表格列自动对齐

## 更新日志

### v0.7.0 (当前版本)
- ✅ **内存池扩展**：将内存池从 100MB 扩展到 1GB
  - 修复栈溢出问题（使用 `malloc` 替代 `std::vector`，更底层可控）
  - 支持动态分配元信息数组（使用 `malloc` + placement new）
  - 当前配置：1GB（262,144 个 4KB 块）
- ✅ **内存紧凑算法优化**：修复 compact 函数实现
  - 按 `memory_id` 为单位整体移动，确保同一内存的所有块连续移动
  - 按原始位置排序处理，确保移动顺序正确
  - 修复多块内存的移动问题
- ✅ **文件上传功能增强**：支持中文路径
  - 支持 UTF-8 编码的中文文件路径
  - 使用 Windows API 处理宽字符路径
- ✅ **文件上传功能**：`alloc` 命令支持从文件上传内容（`.txt` 格式）
  - 支持相对路径和绝对路径
  - 支持带引号的路径格式
  - 自动检测和移除 UTF-8 BOM
- ✅ **UTF-8 中文支持**：完整支持中文显示和文件读取
  - 控制台代码页设置为 UTF-8
  - 文件读取支持 UTF-8 编码
  - 表格显示支持中文对齐
- ✅ **表格显示优化**：改进 `status` 命令的输出格式
  - 添加 Bytes 列（显示实际数据大小）
  - 支持中文列对齐（中文字符按 2 个显示宽度计算）
- ✅ **read 命令优化**：改进输出格式
  - 使用虚线分隔元信息和内容
  - 内容原样输出，不添加引号
  - 支持多行内容和中文显示
- ✅ **compact 函数修复**：修复多块内存的起始位置更新问题
  - 确保 compact 后所有已使用的块连续排列
  - 修复了块范围显示错误的问题

### v0.6.0
- ✅ **文件上传功能**：`alloc` 命令支持从文件上传内容（`.txt` 格式）
- ✅ **UTF-8 中文支持**：完整支持中文显示和文件读取
- ✅ **表格显示优化**：改进 `status` 命令的输出格式
- ✅ **read 命令优化**：改进输出格式
- ✅ **compact 函数修复**：修复多块内存的起始位置更新问题

### v0.5.0
- ✅ **TCP 服务器功能**：完整实现 TCP 服务器，支持多客户端并发连接
- ✅ **网络协议**：实现二进制协议，支持所有内存管理操作
- ✅ **客户端实现指南**：提供通用的客户端实现文档和参考实现
- ✅ **Memory ID 系统集成**：TCP 服务器完全支持 Memory ID 系统
- ✅ **服务器启动信息**：显示可访问的 IP 地址和端口
- ✅ **Windows 平台优化**：移除 Linux/Unix 相关代码，仅支持 Windows

### v0.4.0
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

**项目状态**：核心功能、持久化、时间追踪、TCP 客户端接口、文件上传功能和 UTF-8 中文支持已完成 ✅

**未来开发计划**：
- ✅ **内存池扩展**：已完成，当前支持 1GB 内存池（已修复栈溢出问题）
- ⏳ **紧凑算法优化**：改进内存紧凑算法，提高效率
- 🔄 **SDK/DLL 打包**：目录结构已重组，C API 包装层已实现（40% 完成）
- ⚠️ **TCP 连接修复**：修复 TCP 连接功能存在的问题
- ⏳ **客户端文件上传**：支持客户端通过 TCP 上传文件到内存池

## 客户端实现

### 快速开始

1. **启动服务器**
   ```bash
   cd server
   .\run.bat
   ```
   服务器启动后会显示可访问的 IP 地址和端口（默认 8888）

2. **实现客户端**

   客户端可以使用任何支持 TCP Socket 的编程语言实现（C、C++、Python、Java、Go 等）。详细实现指南请参考 [客户端实现指南](client/README.md)。

### 协议规范

客户端通过 TCP 协议与服务器通信：

**请求格式：**
```
[命令类型: 1字节] [数据长度: 4字节(大端序)] [数据内容: N字节]
```

**响应格式：**
```
[状态码: 1字节] [数据长度: 4字节(大端序)] [响应数据: N字节]
```

**支持的命令：**
- `ALLOC (0x01)`: 分配内存，数据格式 `description\0content`
- `UPDATE (0x02)`: 更新内容，数据格式 `memory_id\0new_content`
- `DELETE (0x03)`: 删除内存，数据格式 `memory_id`
- `READ (0x04)`: 读取内容，数据格式 `memory_id`
- `STATUS (0x05)`: 查询状态，无数据

### 客户端实现

客户端可以使用任何编程语言实现，只需遵循协议规范即可。详细实现指南请参考 [客户端实现指南](client/README.md)，包含：
- 完整的协议规范
- 多语言实现示例（C/C++、Python、Java、Go 等）
- 操作示例和错误处理
- 测试建议和故障排除
