# TCP 客户端接口设计说明

## 目录

- [TCP 客户端接口设计说明](#tcp-客户端接口设计说明)
  - [目录](#目录)
  - [概述](#概述)
  - [架构设计](#架构设计)
    - [1. 协议层 (`protocol.h/cpp`)](#1-协议层-protocolhcpp)
      - [协议包结构（RFC 风格）](#协议包结构rfc-风格)
        - [1.1 请求包格式（Request Packet）](#11-请求包格式request-packet)
        - [1.2 响应包格式（Response Packet）](#12-响应包格式response-packet)
        - [1.3 字节序（Byte Order）](#13-字节序byte-order)
        - [1.3.1 大文件传输说明](#131-大文件传输说明)
        - [1.4 命令类型（Command Type）](#14-命令类型command-type)
        - [1.5 响应状态码（Response Code）](#15-响应状态码response-code)
        - [1.6 数据内容格式](#16-数据内容格式)
        - [1.7 协议示例](#17-协议示例)
    - [2. 服务器层 (`tcp_server.h/cpp`)](#2-服务器层-tcp_serverhcpp)
  - [外部主机访问配置](#外部主机访问配置)
    - [服务器配置](#服务器配置)
    - [防火墙配置](#防火墙配置)
      - [方式一：通过 Windows 防火墙设置（推荐）](#方式一通过-windows-防火墙设置推荐)
      - [方式二：通过命令行（管理员权限）](#方式二通过命令行管理员权限)
      - [方式三：临时禁用防火墙（仅测试，不推荐）](#方式三临时禁用防火墙仅测试不推荐)
    - [路由器/网关配置（公网访问）](#路由器网关配置公网访问)
    - [验证连接](#验证连接)
      - [1. 检查服务器是否监听](#1-检查服务器是否监听)
      - [2. 测试本地连接](#2-测试本地连接)
      - [3. 测试局域网连接](#3-测试局域网连接)
      - [4. 测试公网连接](#4-测试公网连接)
      - [5. 检查端口监听状态](#5-检查端口监听状态)
    - [常见问题](#常见问题)
      - [Q1: 本地可以连接，但外部主机无法连接](#q1-本地可以连接但外部主机无法连接)
      - [Q2: 连接被拒绝（Connection Refused）](#q2-连接被拒绝connection-refused)
      - [Q3: 连接超时（Connection Timeout）](#q3-连接超时connection-timeout)
  - [完整连接流程](#完整连接流程)
    - [完整流程图](#完整流程图)
    - [阶段 1：服务器启动和监听](#阶段-1服务器启动和监听)
      - [1.1 服务器初始化](#11-服务器初始化)
      - [1.2 服务器绑定和监听](#12-服务器绑定和监听)
    - [阶段 2：客户端连接](#阶段-2客户端连接)
      - [2.1 客户端创建 Socket](#21-客户端创建-socket)
      - [2.2 客户端连接到服务器](#22-客户端连接到服务器)
      - [2.3 服务器接受连接](#23-服务器接受连接)
    - [阶段 3：客户端发送命令](#阶段-3客户端发送命令)
      - [3.1 构建请求包](#31-构建请求包)
      - [3.2 发送请求包](#32-发送请求包)
      - [3.3 服务器接收请求](#33-服务器接收请求)
    - [阶段 4：服务器处理命令](#阶段-4服务器处理命令)
      - [4.1 解析命令](#41-解析命令)
      - [4.2 构建响应包](#42-构建响应包)
      - [4.3 发送响应](#43-发送响应)
    - [阶段 5：客户端接收响应](#阶段-5客户端接收响应)
      - [5.1 接收响应头](#51-接收响应头)
      - [5.2 接收响应数据](#52-接收响应数据)
    - [完整示例：外部主机连接并发送命令](#完整示例外部主机连接并发送命令)
      - [Python 客户端完整示例](#python-客户端完整示例)
      - [执行结果](#执行结果)
  - [使用方法](#使用方法)
    - [在 main.cpp 中集成](#在-maincpp-中集成)
    - [编译选项](#编译选项)
  - [客户端接口使用示例](#客户端接口使用示例)
    - [Python 客户端示例](#python-客户端示例)
  - [注意事项](#注意事项)
  - [扩展功能](#扩展功能)

---

## 概述

TCP 服务器模块设计用于提供客户端接口，允许客户端通过网络连接访问共享内存池系统。系统通过 TCP 协议提供 API 接口，客户端无需单独的程序，只需通过网络连接即可访问。

**当前状态**：✅ 基础框架已实现，支持外部主机连接，使用 Memory ID 系统。

## 架构设计

### 1. 协议层 (`protocol.h/cpp`)

定义了网络通信协议格式，参考 RFC 标准设计。

#### 协议包结构（RFC 风格）

##### 1.1 请求包格式（Request Packet）

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Command Type |              Data Length (32-bit)              |
|    (8-bit)    |              (Network Byte Order)              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                    Data Content (Variable)                    |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

**字段说明**：
- **Command Type (8-bit)**: 命令类型，标识请求的操作类型
- **Data Length (32-bit)**: 数据内容长度（大端序/网络字节序），单位：字节
- **Data Content (Variable)**: 数据内容，长度由 Data Length 字段指定

**总长度**: 5 + Data Length 字节

##### 1.2 响应包格式（Response Packet）

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Response Code |              Data Length (32-bit)              |
|    (8-bit)    |              (Network Byte Order)              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                  Response Data (Variable)                    |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

**字段说明**：
- **Response Code (8-bit)**: 响应状态码，标识操作结果
- **Data Length (32-bit)**: 响应数据长度（大端序/网络字节序），单位：字节
- **Response Data (Variable)**: 响应数据内容，长度由 Data Length 字段指定

**总长度**: 5 + Data Length 字节

##### 1.3 字节序（Byte Order）

- 所有多字节数值字段使用**网络字节序（大端序，Big-Endian）**
- 单字节字段（Command Type、Response Code）不受字节序影响
- 数据长度字段使用 32 位无符号整数（uint32_t）

##### 1.3.1 大文件传输说明

**TCP 数据包 vs TCP 流**：
- **重要澄清**：TCP 是**流式协议**，不是基于固定大小数据包的
- IP 数据包的最大传输单元（MTU）通常是 1500 字节（以太网），但这**不是**TCP 协议的限制
- TCP 会自动将应用层数据分割成多个 IP 数据包传输，并在接收端重组
- 64KB 通常指的是 TCP 窗口大小或其他配置参数，**不是**单个数据包的限制

**协议设计支持大文件**：
- Data Length 字段是 **32 位无符号整数**，理论上支持最大 **4GB** 的数据传输
- TCP 协议本身可以传输任意大小的数据（受内存和网络带宽限制）

**当前实现限制**：
- 当前代码中设置了 **1MB** 的应用层限制（安全措施）
- 如需支持更大的文件，可以：
  1. **提高限制**：修改 `protocol.cpp` 中的 `1024 * 1024` 限制
  2. **分块传输**：实现分块上传机制（推荐，更可靠）
  3. **流式处理**：边接收边写入内存池，避免一次性加载到内存

**分块传输建议**（未来扩展）：
```
1. 客户端将大文件分割成多个块（如 64KB/块）
2. 使用扩展命令（如 ALLOC_CHUNK）逐块上传
3. 服务器端接收并组装
4. 完成后返回完整的 Memory ID
```

**注意事项**：
- 大文件传输时需要考虑：
  - 网络超时设置
  - 接收缓冲区大小
  - 内存池可用空间
  - 传输进度反馈

##### 1.4 命令类型（Command Type）

| 值   | 命令   | 说明          |
| ---- | ------ | ------------- |
| 0x01 | ALLOC  | 分配内存      |
| 0x02 | UPDATE | 更新内容      |
| 0x03 | DELETE | 删除/释放内存 |
| 0x04 | READ   | 读取内容      |
| 0x05 | STATUS | 查询状态      |

##### 1.5 响应状态码（Response Code）

| 值   | 状态码               | 说明       |
| ---- | -------------------- | ---------- |
| 0x00 | SUCCESS              | 操作成功   |
| 0x01 | ERROR_INVALID_CMD    | 无效命令   |
| 0x02 | ERROR_INVALID_PARAM  | 无效参数   |
| 0x03 | ERROR_NO_MEMORY      | 内存不足   |
| 0x04 | ERROR_NOT_FOUND      | 资源不存在 |
| 0x05 | ERROR_ALREADY_EXISTS | 资源已存在 |
| 0xFF | ERROR_INTERNAL       | 内部错误   |

##### 1.6 数据内容格式

**ALLOC 命令**：
```
Data = Description + '\0' + Content
```
- Description: 描述信息（UTF-8 字符串）
- '\0': 分隔符（1字节）
- Content: 内容数据（二进制数据，支持任意内容）

**UPDATE 命令**：
```
Data = MemoryID + '\0' + NewContent
```
- MemoryID: 内存标识符（如 "memory_00001"）
- '\0': 分隔符（1字节）
- NewContent: 新内容（二进制数据）

**DELETE/READ 命令**：
```
Data = MemoryID
```
- MemoryID: 内存标识符（如 "memory_00001"）

**STATUS 命令**：
```
Data = "" (空字符串)
```

##### 1.7 协议示例

**示例 1：ALLOC 请求**
```
Command Type: 0x01
Data Length:  0x00000014 (20 bytes)
Data:         "User Data\0Hello World"
              (Description: "User Data", Content: "Hello World")
```

**示例 2：READ 响应（成功）**
```
Response Code: 0x00 (SUCCESS)
Data Length:   0x0000000B (11 bytes)
Data:          "Hello World"
```

**示例 3：DELETE 响应（失败）**
```
Response Code: 0x04 (ERROR_NOT_FOUND)
Data Length:   0x0000001A (26 bytes)
Data:          "Memory ID not found"
```

### 2. 服务器层 (`tcp_server.h/cpp`)

**设计目标**：
- 监听指定端口（默认8888）
- 接受客户端连接
- 为每个连接创建独立处理线程
- 处理客户端请求并返回响应
- 使用 Memory ID 系统管理内存（所有客户端共享访问）

**工作流程**：
```
1. 启动服务器 → 绑定端口 → 开始监听
2. 接受客户端连接 → 创建处理线程
3. 接收请求 → 解析协议 → 调用内存池接口（使用 Memory ID） → 发送响应
4. 客户端断开 → 清理资源（不清理内存，内存由 Memory ID 管理）
```

**当前实现状态**：
- ✅ TCP 服务器基础框架已实现
- ✅ 网络协议定义已完成
- ✅ 支持外部主机连接（绑定到 0.0.0.0）
- ✅ 使用 Memory ID 系统

## 外部主机访问配置

### 服务器配置

服务器代码中已经配置为接受外部连接：

```cpp
// server/network/tcp_server.cpp
serverAddr.sin_addr.s_addr = INADDR_ANY;  // 绑定到所有网络接口
```

这意味着：
- ✅ **本地连接**：可以通过 `127.0.0.1:8888` 连接
- ✅ **局域网连接**：可以通过局域网 IP（如 `192.168.1.100:8888`）连接
- ✅ **公网连接**：如果有公网 IP，可以通过公网 IP 连接

### 防火墙配置

要让外部主机能够连接，需要配置 Windows 防火墙：

#### 方式一：通过 Windows 防火墙设置（推荐）

1. 打开"Windows Defender 防火墙"
2. 点击"高级设置"
3. 选择"入站规则" → "新建规则"
4. 选择"端口" → 下一步
5. 选择"TCP"，输入端口 `8888` → 下一步
6. 选择"允许连接" → 下一步
7. 勾选所有配置文件（域、专用、公用）→ 下一步
8. 输入规则名称，如"Shared Memory Management Server" → 完成

#### 方式二：通过命令行（管理员权限）

```powershell
# 允许入站连接（TCP 8888）
netsh advfirewall firewall add rule name="SMM Server" dir=in action=allow protocol=TCP localport=8888

# 查看规则
netsh advfirewall firewall show rule name="SMM Server"
```

#### 方式三：临时禁用防火墙（仅测试，不推荐）

```powershell
# 临时禁用防火墙（仅用于测试）
netsh advfirewall set allprofiles state off

# 重新启用防火墙
netsh advfirewall set allprofiles state on
```

### 路由器/网关配置（公网访问）

如果要从互联网访问，还需要配置路由器端口转发：

1. **获取服务器内网 IP**：
   ```bash
   ipconfig
   # 找到 IPv4 地址，如 192.168.1.100
   ```

2. **配置路由器端口转发**：
   - 登录路由器管理界面
   - 找到"端口转发"或"虚拟服务器"设置
   - 添加规则：
     - 外部端口：8888
     - 内部 IP：服务器内网 IP（如 192.168.1.100）
     - 内部端口：8888
     - 协议：TCP

3. **获取公网 IP**：
   - 访问 https://www.whatismyip.com/ 查看公网 IP
   - 客户端使用公网 IP 连接

### 验证连接

#### 1. 检查服务器是否监听

服务器启动时会显示可访问的 IP 地址：

```
Server is ready for client connections:

  Private IP (LAN access):
    192.168.20.31:8888 (LAN access)
    172.29.56.238:8888 (LAN access)

  Local only:  127.0.0.1:8888 (Local access only)
```

#### 2. 测试本地连接

```bash
# 使用 telnet 测试（Windows）
telnet 127.0.0.1 8888

# 或使用 PowerShell
Test-NetConnection -ComputerName 127.0.0.1 -Port 8888
```

#### 3. 测试局域网连接

从同一局域网的另一台机器：

```bash
# 使用服务器的局域网 IP
telnet 192.168.1.100 8888
```

#### 4. 测试公网连接

从互联网上的另一台机器：

```bash
# 使用服务器的公网 IP
telnet <公网IP> 8888
```

#### 5. 检查端口监听状态

```powershell
# Windows 查看端口监听
netstat -an | findstr :8888

# 应该看到：
# TCP    0.0.0.0:8888           0.0.0.0:0              LISTENING
# 如果看到 127.0.0.1:8888，说明只监听本地
```

### 常见问题

#### Q1: 本地可以连接，但外部主机无法连接

**可能原因**：
1. 防火墙阻止了连接
2. 路由器未配置端口转发（公网访问）
3. 服务器绑定了错误的地址

**解决方法**：
1. 检查防火墙规则（见上方配置）
2. 检查路由器端口转发设置
3. 确认服务器使用 `INADDR_ANY`（代码已正确配置）

#### Q2: 连接被拒绝（Connection Refused）

**可能原因**：
1. 服务器未启动
2. 端口被其他程序占用
3. 防火墙阻止

**解决方法**：
1. 确认服务器正在运行
2. 检查端口占用：`netstat -ano | findstr :8888`
3. 检查防火墙设置

#### Q3: 连接超时（Connection Timeout）

**可能原因**：
1. 防火墙阻止
2. 路由器未配置端口转发
3. 网络不通

**解决方法**：
1. 检查防火墙规则
2. 检查路由器配置
3. 使用 `ping` 测试网络连通性

## 完整连接流程

### 完整流程图

```
┌─────────────┐                                    ┌─────────────┐
│  客户端主机  │                                    │  服务器主机  │
│ (外部主机)   │                                    │              │
└──────┬──────┘                                    └──────┬──────┘
       │                                                   │
       │  1. 创建 TCP Socket                              │
       │  2. 连接到服务器 IP:Port                          │
       │  ───────────────────────────────────────────────>│
       │                                                   │  3. 服务器 accept() 接受连接
       │                                                   │  4. 创建客户端处理线程
       │                                                   │
       │  5. 构建请求包                                    │
       │     [命令类型: 1字节]                            │
       │     [数据长度: 4字节(大端序)]                    │
       │     [数据内容: N字节]                            │
       │  ───────────────────────────────────────────────>│
       │                                                   │  6. 接收请求包
       │                                                   │  7. 解析协议
       │                                                   │  8. 处理命令（调用内存池）
       │                                                   │  9. 构建响应包
       │                                                   │     [状态码: 1字节]
       │                                                   │     [数据长度: 4字节(大端序)]
       │                                                   │     [响应数据: N字节]
       │  <───────────────────────────────────────────────│
       │  10. 接收响应包                                   │
       │  11. 解析响应                                     │
       │  12. 处理结果                                     │
       │                                                   │
       │  (可以继续发送更多命令...)                        │
       │                                                   │
       │  13. 关闭连接                                     │
       │  ───────────────────────────────────────────────>│
       │                                                   │  14. 清理资源
```

### 阶段 1：服务器启动和监听

#### 1.1 服务器初始化

```cpp
// server/main.cpp
TCPServer tcpServer(smp, 8888);  // 创建 TCP 服务器，端口 8888
tcpServer.Start();                // 启动服务器
```

#### 1.2 服务器绑定和监听

```cpp
// server/network/tcp_server.cpp - Start() 函数

// 步骤 1: 创建套接字
listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

// 步骤 2: 设置套接字选项（允许地址重用）
setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, ...);

// 步骤 3: 绑定到所有网络接口（0.0.0.0）
sockaddr_in serverAddr{};
serverAddr.sin_family = AF_INET;
serverAddr.sin_addr.s_addr = INADDR_ANY;  // 关键：绑定到所有接口
serverAddr.sin_port = htons(8888);
bind(listenSocket_, ...);

// 步骤 4: 开始监听
listen(listenSocket_, SOMAXCONN);

// 步骤 5: 进入 accept() 循环，等待客户端连接
while (running_) {
    SOCKET clientSocket = accept(listenSocket_, ...);
    // 为每个客户端创建处理线程
}
```

**服务器状态**：
- ✅ 监听端口：8888
- ✅ 绑定地址：0.0.0.0（所有网络接口）
- ✅ 等待连接：`accept()` 阻塞等待客户端连接

### 阶段 2：客户端连接

#### 2.1 客户端创建 Socket

**Python 示例**：
```python
import socket

# 创建 TCP Socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
```

**C++ 示例**：
```cpp
SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
```

#### 2.2 客户端连接到服务器

**Python 示例**：
```python
# 连接到服务器（使用服务器的 IP 地址，不是 localhost）
server_ip = "192.168.1.100"  # 服务器的局域网 IP 或公网 IP
server_port = 8888
sock.connect((server_ip, server_port))
```

**C++ 示例**：
```cpp
sockaddr_in serverAddr{};
serverAddr.sin_family = AF_INET;
serverAddr.sin_port = htons(8888);
inet_pton(AF_INET, "192.168.1.100", &serverAddr.sin_addr);
connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
```

**网络层交互（TCP 三次握手）**：
```
客户端                   服务器
  │                        │
  │  SYN (seq=x)           │
  ├───────────────────────>│
  │                        │  accept() 返回新的 clientSocket
  │  SYN-ACK (seq=y, ack=x+1)│
  │<───────────────────────┤
  │                        │
  │  ACK (ack=y+1)         │
  ├───────────────────────>│
  │                        │  创建客户端处理线程
  │                        │  HandleClient(clientSocket, ...)
```

#### 2.3 服务器接受连接

```cpp
// server/network/tcp_server.cpp

// accept() 返回新的客户端套接字
SOCKET clientSocket = accept(listenSocket_, ...);

// 获取客户端 IP 和端口
char ipStr[INET_ADDRSTRLEN];
inet_ntoa(clientAddr.sin_addr);  // 获取客户端 IP
ntohs(clientAddr.sin_port);      // 获取客户端端口

std::cout << "Client connected: " << clientIP << ":" << clientPort << "\n";

// 为客户端创建处理线程
clientThreads_.emplace_back([this, clientSocket, clientIP, clientPort]() {
    HandleClient(clientSocket, clientIP, clientPort);
});
```

**连接建立后的状态**：
- ✅ TCP 连接已建立
- ✅ 服务器创建了客户端处理线程
- ✅ 客户端可以开始发送命令

### 阶段 3：客户端发送命令

#### 3.1 构建请求包

**协议格式**：
```
[命令类型: 1字节] [数据长度: 4字节(大端序)] [数据内容: N字节]
```

**Python 示例 - ALLOC 命令**：
```python
# 命令类型
cmd_type = 0x01  # ALLOC

# 数据内容：description\0content
description = "User Data"
content = "Hello World"
data = description.encode('utf-8') + b'\0' + content.encode('utf-8')

# 构建请求包
cmd_byte = cmd_type.to_bytes(1, 'big')           # 1 字节
data_len = len(data).to_bytes(4, 'big')          # 4 字节（大端序）
request_packet = cmd_byte + data_len + data       # 完整请求包
```

**字节级示例**：
```
请求包内容（十六进制）：
01 00 00 00 14 55 73 65 72 20 44 61 74 61 00 48 65 6C 6C 6F 20 57 6F 72 6C 64

解析：
01                    - 命令类型：ALLOC (0x01)
00 00 00 14          - 数据长度：20 字节（大端序）
55 73 65 72 20 44 61 74 61 00 48 65 6C 6C 6F 20 57 6F 72 6C 64
                     - 数据内容："User Data\0Hello World"
```

#### 3.2 发送请求包

**Python 示例**：
```python
# 发送完整请求包
sock.sendall(request_packet)
```

#### 3.3 服务器接收请求

```cpp
// server/network/tcp_server.cpp - ReceiveRequest()

// 步骤 1: 接收请求头（5 字节）
uint8_t header[5];
recv(clientSocket, header, 5, 0);
// header[0] = 命令类型
// header[1-4] = 数据长度（大端序）

// 步骤 2: 解析数据长度
uint32_t netLen;
memcpy(&netLen, header + 1, 4);
uint32_t dataLen = ntohl(netLen);  // 转换为主机字节序

// 步骤 3: 接收数据内容
std::vector<uint8_t> data(dataLen);
recv(clientSocket, data.data(), dataLen, 0);

// 步骤 4: 反序列化请求
Protocol::DeserializeRequest(fullPacket, req);
```

### 阶段 4：服务器处理命令

#### 4.1 解析命令

```cpp
// server/network/tcp_server.cpp - ProcessRequest()

switch (req.cmd) {
    case Protocol::CommandType::ALLOC: {
        // 数据格式：description\0content
        size_t nullPos = req.data.find('\0');
        std::string description = req.data.substr(0, nullPos);
        std::string content = req.data.substr(nullPos + 1);
        
        // 生成 Memory ID
        std::string memory_id = smp_.GenerateNextMemoryId();
        
        // 分配内存
        int blockID = smp_.AllocateBlock(memory_id, description, 
                                         content.data(), content.size());
        
        if (blockID < 0) {
            resp.code = Protocol::ResponseCode::ERROR_NO_MEMORY;
            resp.data = "Memory allocation failed";
        } else {
            resp.code = Protocol::ResponseCode::SUCCESS;
            resp.data = memory_id;  // 返回 Memory ID
        }
        break;
    }
    
    case Protocol::CommandType::READ: {
        // 数据格式：memory_id
        std::string memory_id = req.data;
        
        // 读取内存内容
        std::string content = smp_.GetMemoryContentAsString(memory_id);
        
        if (content.empty()) {
            resp.code = Protocol::ResponseCode::ERROR_NOT_FOUND;
            resp.data = "Memory ID not found";
        } else {
            resp.code = Protocol::ResponseCode::SUCCESS;
            resp.data = content;
        }
        break;
    }
    
    // ... 其他命令
}
```

#### 4.2 构建响应包

**协议格式**：
```
[状态码: 1字节] [数据长度: 4字节(大端序)] [响应数据: N字节]
```

```cpp
// server/network/protocol.cpp - SerializeResponse()

std::vector<uint8_t> buffer;
buffer.push_back(static_cast<uint8_t>(resp.code));  // 状态码

uint32_t len = resp.data.size();
uint32_t netLen = htonl(len);  // 转换为网络字节序
const uint8_t* lenBytes = reinterpret_cast<const uint8_t*>(&netLen);
buffer.insert(buffer.end(), lenBytes, lenBytes + 4);  // 数据长度

buffer.insert(buffer.end(), resp.data.begin(), resp.data.end());  // 数据内容
```

**响应包示例（ALLOC 成功）**：
```
00 00 00 00 0D 6D 65 6D 6F 72 79 5F 30 30 30 30 31

解析：
00                    - 状态码：SUCCESS (0x00)
00 00 00 0D          - 数据长度：13 字节（大端序）
6D 65 6D 6F 72 79 5F 30 30 30 30 31
                     - 响应数据："memory_00001"
```

#### 4.3 发送响应

```cpp
// server/network/tcp_server.cpp - SendResponse()

send(clientSocket, response_packet.data(), response_packet.size(), 0);
```

### 阶段 5：客户端接收响应

#### 5.1 接收响应头

**Python 示例**：
```python
# 接收响应头（5 字节）
header = sock.recv(5)
if len(header) < 5:
    raise RuntimeError("Incomplete response header")

# 解析状态码和数据长度
status_code = header[0]
data_len = struct.unpack('>I', header[1:5])[0]  # 大端序无符号整数
```

#### 5.2 接收响应数据

```python
# 接收响应数据（可能分多次接收）
response_data = b""
while len(response_data) < data_len:
    chunk = sock.recv(data_len - len(response_data))
    if not chunk:
        raise RuntimeError("Incomplete response data")
    response_data += chunk

# 解析响应
if status_code == 0x00:  # SUCCESS
    result = response_data.decode('utf-8')
    print(f"Success: {result}")
else:
    error_msg = response_data.decode('utf-8')
    print(f"Error [{status_code}]: {error_msg}")
```

### 完整示例：外部主机连接并发送命令

#### Python 客户端完整示例

```python
import socket
import struct

# 步骤 1: 创建 Socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# 步骤 2: 连接到服务器（使用服务器 IP，不是 localhost）
server_ip = "192.168.1.100"  # 服务器的 IP 地址
server_port = 8888
print(f"Connecting to {server_ip}:{server_port}...")
sock.connect((server_ip, server_port))
print("Connected!")

# 步骤 3: 发送 ALLOC 命令
print("\n=== Sending ALLOC command ===")
cmd_type = 0x01  # ALLOC
description = "User Data"
content = "Hello World"
data = description.encode('utf-8') + b'\0' + content.encode('utf-8')

# 构建请求包
request = cmd_type.to_bytes(1, 'big') + len(data).to_bytes(4, 'big') + data
sock.sendall(request)

# 接收响应
header = sock.recv(5)
status = header[0]
data_len = struct.unpack('>I', header[1:5])[0]
response_data = sock.recv(data_len).decode('utf-8')

if status == 0x00:
    print(f"ALLOC Success! Memory ID: {response_data}")
    memory_id = response_data
else:
    print(f"ALLOC Failed: {response_data}")
    sock.close()
    exit(1)

# 步骤 4: 发送 READ 命令
print("\n=== Sending READ command ===")
cmd_type = 0x04  # READ
data = memory_id.encode('utf-8')

request = cmd_type.to_bytes(1, 'big') + len(data).to_bytes(4, 'big') + data
sock.sendall(request)

# 接收响应
header = sock.recv(5)
status = header[0]
data_len = struct.unpack('>I', header[1:5])[0]
response_data = sock.recv(data_len).decode('utf-8')

if status == 0x00:
    print(f"READ Success! Content: {response_data}")
else:
    print(f"READ Failed: {response_data}")

# 步骤 5: 关闭连接
print("\nClosing connection...")
sock.close()
print("Disconnected")
```

#### 执行结果

**客户端输出**：
```
Connecting to 192.168.1.100:8888...
Connected!

=== Sending ALLOC command ===
ALLOC Success! Memory ID: memory_00001

=== Sending READ command ===
READ Success! Content: Hello World

Closing connection...
Disconnected
```

**服务器端输出**：
```
TCP Server started on port 8888
Client connected: 192.168.1.50:54321
Client disconnected: 192.168.1.50:54321
```

## 使用方法

### 在 main.cpp 中集成

```cpp
#include "network/tcp_server.h"

int main() {
    SharedMemoryPool smp;
    // ... 初始化代码 ...
    
    // 创建TCP服务器（端口8888）
    TCPServer tcpServer(smp, 8888);
    
    // 在单独线程中启动TCP服务器
    std::thread serverThread([&tcpServer]() {
        tcpServer.Start(); // 阻塞调用
    });
    
    // 原有的REPL循环
    while (true) {
        // ... REPL代码 ...
    }
    
    // 停止TCP服务器
    tcpServer.Stop();
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    return 0;
}
```

### 编译选项

在 `run.bat` 中添加网络模块：

```batch
g++ -std=c++17 -Wall main.cpp ^
    command/commands.cpp ^
    shared_memory_pool/shared_memory_pool.cpp ^
    persistence/persistence.cpp ^
    network/protocol.cpp ^
    network/tcp_server.cpp ^
    -o main.exe -lws2_32
```

注意：Windows 平台需要链接 `ws2_32.lib`（通过 `-lws2_32` 或 `#pragma comment`）

## 客户端接口使用示例

### Python 客户端示例

```python
import socket
import struct

def send_request(sock, cmd_type, data):
    # 序列化请求
    cmd_byte = cmd_type.to_bytes(1, 'big')
    data_len = len(data).to_bytes(4, 'big')
    packet = cmd_byte + data_len + data.encode('utf-8')
    sock.sendall(packet)
    
    # 接收响应
    header = sock.recv(5)
    if len(header) < 5:
        return None
    
    status = header[0]
    resp_len = struct.unpack('>I', header[1:5])[0]
    resp_data = sock.recv(resp_len).decode('utf-8')
    
    return status, resp_data

# 连接服务器（使用服务器 IP，不是 localhost）
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.1.100', 8888))  # 使用服务器 IP

# 分配内存（需要提供 description 和 content）
# 协议格式：description + "\0" + content
alloc_data = "User Data\0Hello World"
status, resp = send_request(sock, 0x01, alloc_data)
print(f"ALLOC: {status}, {resp}")  # 返回 Memory ID

# 读取内容（使用 Memory ID）
status, resp = send_request(sock, 0x04, "memory_00001")
print(f"READ: {status}, {resp}")

# 更新内容（使用 Memory ID）
update_data = "Updated Content"
status, resp = send_request(sock, 0x02, "memory_00001\0" + update_data)
print(f"UPDATE: {status}, {resp}")

# 释放内存（使用 Memory ID）
status, resp = send_request(sock, 0x03, "memory_00001")
print(f"DELETE: {status}, {resp}")

sock.close()
```

## 注意事项

1. **客户端接口**：系统通过 TCP 服务器提供客户端接口，客户端通过网络连接访问，无需单独的程序
2. **外部访问**：服务器已配置为接受外部主机连接，但需要配置防火墙和路由器（如需要）
3. **Memory ID 系统**：使用 `memory_00001`, `memory_00002` 等唯一标识符，所有客户端共享访问
4. **线程安全**：每个客户端连接在独立线程中处理，内存池操作需要保证线程安全
5. **错误处理**：网络操作可能失败，需要适当的错误处理
6. **资源管理**：客户端断开时不会自动清理内存，内存由 Memory ID 管理
7. **粘包问题**：当前实现已处理粘包（通过读取完整数据长度）
8. **协议格式**：ALLOC 命令需要提供 description 和 content，用 `\0` 分隔
9. **数据接收**：TCP 流式传输，数据可能分多次到达，需要循环接收直到完整
10. **字节序**：必须使用网络字节序（大端序）进行数据长度编码/解码

## 扩展功能

可以添加的功能：
- 连接超时检测
- 连接数限制
- 请求频率限制
- SSL/TLS 加密支持
