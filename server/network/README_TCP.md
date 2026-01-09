# TCP 服务器实现说明

## 概述

TCP 服务器模块提供了网络访问接口，允许客户端通过网络连接访问共享内存池系统。

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

**核心功能**：
- 监听指定端口（默认8888）
- 接受客户端连接
- 为每个连接创建独立处理线程
- 从连接获取客户端 IP:Port 作为用户标识
- 处理客户端请求并返回响应

**工作流程**：
```
1. 启动服务器 → 绑定端口 → 开始监听
2. 接受连接 → 获取客户端IP:Port → 创建用户ID
3. 创建处理线程 → 接收请求 → 处理请求 → 发送响应
4. 客户端断开 → 清理资源
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

## 客户端实现示例

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

# 分配内存
status, resp = send_request(sock, 0x01, "Hello World")
print(f"ALLOC: {status}, {resp}")

# 读取内容
status, resp = send_request(sock, 0x04, "")
print(f"READ: {status}, {resp}")

sock.close()
```

## 注意事项

1. **线程安全**：每个客户端连接在独立线程中处理，内存池操作需要保证线程安全
2. **错误处理**：网络操作可能失败，需要适当的错误处理
3. **资源管理**：客户端断开时需要清理资源
4. **粘包问题**：当前实现已处理粘包（通过读取完整数据长度）
5. **用户标识**：使用 `IP:Port` 作为用户标识，确保唯一性

## 扩展功能

可以添加的功能：
- 连接超时检测
- 心跳机制（PING/PONG）
- 连接数限制
- 请求频率限制
- SSL/TLS 加密支持
