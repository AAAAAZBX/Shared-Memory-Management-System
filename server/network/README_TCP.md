# TCP 客户端接口设计说明

## 概述

TCP 服务器模块设计用于提供客户端接口，允许客户端通过网络连接访问共享内存池系统。系统通过 TCP 协议提供 API 接口，客户端无需单独的程序，只需通过网络连接即可访问。

**当前状态**：基础框架已实现，但需要更新为 Memory ID 系统并集成到主程序。

## 架构设计

### 1. 协议层 (`protocol.h/cpp`)

定义了网络通信协议格式：

**请求格式**：
```
[命令类型: 1字节] [数据长度: 4字节(大端序)] [数据内容: N字节]
```

**响应格式**：
```
[状态码: 1字节] [数据长度: 4字节(大端序)] [响应数据: N字节]
```

**支持的命令**：
- `ALLOC (0x01)`: 分配内存
- `UPDATE (0x02)`: 更新内容
- `DELETE (0x03)`: 删除/释放内存
- `READ (0x04)`: 读取内容
- `STATUS (0x05)`: 查询状态
- `PING (0x06)`: 心跳检测

### 2. 服务器层 (`tcp_server.h/cpp`)

**设计目标**：
- 监听指定端口（默认8888）
- 接受客户端连接
- 为每个连接创建独立处理线程
- 处理客户端请求并返回响应
- 使用 Memory ID 系统管理内存（所有客户端共享访问，不再使用 IP:Port 作为用户标识）

**工作流程**（待实现）：
```
1. 启动服务器 → 绑定端口 → 开始监听
2. 接受客户端连接 → 创建处理线程
3. 接收请求 → 解析协议 → 调用内存池接口（使用 Memory ID） → 发送响应
4. 客户端断开 → 清理资源（不清理内存，内存由 Memory ID 管理）
```

**当前实现状态**：
- ✅ TCP 服务器基础框架已实现
- ✅ 网络协议定义已完成
- ⏳ 待更新：ProcessRequest 需要更新为 Memory ID 系统
- ⏳ 待集成：需要集成到 main.cpp

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

# 连接服务器
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 8888))

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
2. **Memory ID 系统**：使用 `memory_00001`, `memory_00002` 等唯一标识符，所有客户端共享访问
3. **线程安全**：每个客户端连接在独立线程中处理，内存池操作需要保证线程安全
4. **错误处理**：网络操作可能失败，需要适当的错误处理
5. **资源管理**：客户端断开时不会自动清理内存，内存由 Memory ID 管理
6. **粘包问题**：当前实现已处理粘包（通过读取完整数据长度）
7. **协议格式**：ALLOC 命令需要提供 description 和 content，用 `\0` 分隔

## 扩展功能

可以添加的功能：
- 连接超时检测
- 心跳机制（PING/PONG）
- 连接数限制
- 请求频率限制
- SSL/TLS 加密支持
