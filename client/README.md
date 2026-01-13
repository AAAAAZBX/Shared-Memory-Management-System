# 客户端实现指南

## 概述

本文档提供客户端实现的通用指南，适用于任何支持 TCP Socket 的编程语言（C、C++、Python、Java、Go 等）。客户端通过 TCP 协议与服务器通信，实现共享内存池的远程访问。

## 环境要求

- 支持 TCP Socket 的编程语言
- 网络连接（本地或远程）
- 了解二进制数据序列化/反序列化

## 协议规范

### 连接信息

- **协议**：TCP
- **默认端口**：8888
- **字节序**：大端序（Network Byte Order / Big-Endian）

### 请求包格式

```
[命令类型: 1字节] [数据长度: 4字节(大端序)] [数据内容: N字节]
```

**字段说明：**
- **命令类型**（1字节）：无符号整数，表示操作类型
- **数据长度**（4字节）：大端序无符号整数，表示数据内容的字节数
- **数据内容**（N字节）：实际数据，UTF-8 编码的字符串

### 响应包格式

```
[状态码: 1字节] [数据长度: 4字节(大端序)] [响应数据: N字节]
```

**字段说明：**
- **状态码**（1字节）：无符号整数，表示操作结果
- **数据长度**（4字节）：大端序无符号整数，表示响应数据的字节数
- **响应数据**（N字节）：实际响应内容，UTF-8 编码的字符串

### 命令类型定义

| 命令     | 值   | 数据格式                 | 说明                     |
| -------- | ---- | ------------------------ | ------------------------ |
| `ALLOC`  | 0x01 | `description\0content`   | 分配内存，返回 Memory ID |
| `UPDATE` | 0x02 | `memory_id\0new_content` | 更新内存内容             |
| `DELETE` | 0x03 | `memory_id`              | 删除/释放内存            |
| `READ`   | 0x04 | `memory_id`              | 读取内存内容             |
| `STATUS` | 0x05 | 无（空字符串）           | 查询所有内存块状态       |
| `PING`   | 0x06 | 无（空字符串）           | 心跳检测                 |

### 状态码定义

| 状态码                 | 值   | 说明                 |
| ---------------------- | ---- | -------------------- |
| `SUCCESS`              | 0x00 | 操作成功             |
| `ERROR_INVALID_CMD`    | 0x01 | 无效命令             |
| `ERROR_INVALID_PARAM`  | 0x02 | 无效参数             |
| `ERROR_NO_MEMORY`      | 0x03 | 内存不足             |
| `ERROR_NOT_FOUND`      | 0x04 | Memory ID 不存在     |
| `ERROR_ALREADY_EXISTS` | 0x05 | 已存在（当前未使用） |
| `ERROR_INTERNAL`       | 0xFF | 服务器内部错误       |

## 实现步骤

### 1. 建立 TCP 连接

使用标准 TCP Socket API 连接到服务器。

**示例（伪代码）：**
```c
socket = socket(AF_INET, SOCK_STREAM, 0);
connect(socket, server_address, port);
```

### 2. 发送请求

#### 步骤：
1. 构建数据内容（根据命令类型）
2. 计算数据长度（字节数）
3. 将命令类型、长度、数据打包发送

#### 数据格式说明：

**ALLOC 命令：**
```
数据 = description + '\0' + content
例如: "My Data\0Hello World"
```

**UPDATE 命令：**
```
数据 = memory_id + '\0' + new_content
例如: "memory_00001\0New Content"
```

**DELETE/READ 命令：**
```
数据 = memory_id
例如: "memory_00001"
```

**STATUS/PING 命令：**
```
数据 = "" (空字符串，长度为0)
```

#### 发送示例（伪代码）：
```c
// 构建请求包
uint8_t cmd = 0x01;  // ALLOC
char* data = "My Data\0Hello World";
uint32_t data_len = strlen(data);

// 转换为大端序
uint32_t net_len = htonl(data_len);

// 发送
send(socket, &cmd, 1, 0);
send(socket, &net_len, 4, 0);
send(socket, data, data_len, 0);
```

### 3. 接收响应

#### 步骤：
1. 接收响应头（5字节）
2. 解析状态码和数据长度
3. 根据数据长度接收完整响应数据

#### 接收示例（伪代码）：
```c
// 接收响应头（5字节）
uint8_t header[5];
recv(socket, header, 5, 0);

// 解析
uint8_t code = header[0];
uint32_t net_len = *(uint32_t*)(header + 1);
uint32_t data_len = ntohl(net_len);

// 接收响应数据
char* response_data = malloc(data_len + 1);
recv(socket, response_data, data_len, 0);
response_data[data_len] = '\0';
```

## 各语言实现参考

### C/C++ 实现

**关键函数：**
- `socket()`, `connect()`: 建立连接
- `send()`, `recv()`: 发送/接收数据
- `htonl()`, `ntohl()`: 字节序转换

**示例代码片段：**
```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// 连接服务器
int sock = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(8888);
inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
connect(sock, (struct sockaddr*)&addr, sizeof(addr));

// 发送请求
uint8_t cmd = 0x04;  // READ
const char* memory_id = "memory_00001";
uint32_t len = strlen(memory_id);
uint32_t net_len = htonl(len);

send(sock, &cmd, 1, 0);
send(sock, &net_len, 4, 0);
send(sock, memory_id, len, 0);

// 接收响应
uint8_t header[5];
recv(sock, header, 5, 0);
uint8_t code = header[0];
uint32_t resp_len = ntohl(*(uint32_t*)(header + 1));
char* response = malloc(resp_len + 1);
recv(sock, response, resp_len, 0);
response[resp_len] = '\0';
```

### Python 实现

**关键模块：**
- `socket`: TCP 连接
- `struct`: 字节序转换

**示例代码片段：**
```python
import socket
import struct

# 连接服务器
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(("127.0.0.1", 8888))

# 发送请求
cmd = 0x04  # READ
memory_id = "memory_00001"
data = memory_id.encode('utf-8')
data_len = len(data)

sock.send(bytes([cmd]))
sock.send(struct.pack('>I', data_len))  # 大端序
sock.send(data)

# 接收响应
header = sock.recv(5)
code = header[0]
resp_len = struct.unpack('>I', header[1:5])[0]  # 大端序
response = sock.recv(resp_len).decode('utf-8')
```

### Java 实现

**关键类：**
- `Socket`: TCP 连接
- `DataInputStream`, `DataOutputStream`: 数据读写
- `ByteBuffer`: 字节序转换

**示例代码片段：**
```java
import java.net.Socket;
import java.io.*;

// 连接服务器
Socket socket = new Socket("127.0.0.1", 8888);
DataOutputStream out = new DataOutputStream(socket.getOutputStream());
DataInputStream in = new DataInputStream(socket.getInputStream());

// 发送请求
byte cmd = 0x04;  // READ
String memoryId = "memory_00001";
byte[] data = memoryId.getBytes("UTF-8");

out.writeByte(cmd);
out.writeInt(data.length);  // 自动大端序
out.write(data);

// 接收响应
byte code = in.readByte();
int respLen = in.readInt();  // 自动大端序
byte[] response = new byte[respLen];
in.readFully(response);
String result = new String(response, "UTF-8");
```

### Go 实现

**关键包：**
- `net`: TCP 连接
- `encoding/binary`: 字节序转换

**示例代码片段：**
```go
import (
    "net"
    "encoding/binary"
)

// 连接服务器
conn, _ := net.Dial("tcp", "127.0.0.1:8888")
defer conn.Close()

// 发送请求
cmd := byte(0x04)  // READ
memoryId := "memory_00001"
data := []byte(memoryId)

conn.Write([]byte{cmd})
binary.Write(conn, binary.BigEndian, uint32(len(data)))
conn.Write(data)

// 接收响应
var code byte
var respLen uint32
conn.Read([]byte{code})
binary.Read(conn, binary.BigEndian, &respLen)
response := make([]byte, respLen)
conn.Read(response)
```

## 操作示例

### 1. 分配内存（ALLOC）

**请求：**
```
命令: 0x01
数据: "My Data\0Hello World"
```

**响应（成功）：**
```
状态码: 0x00
数据: "memory_00001"
```

### 2. 读取内存（READ）

**请求：**
```
命令: 0x04
数据: "memory_00001"
```

**响应（成功）：**
```
状态码: 0x00
数据: "Memory ID: memory_00001\nDescription: My Data\nContent: Hello World\nSize: 11 bytes\nLast Modified: 2024-01-15 10:30:45"
```

### 3. 更新内存（UPDATE）

**请求：**
```
命令: 0x02
数据: "memory_00001\0New Content"
```

**响应（成功）：**
```
状态码: 0x00
数据: "Updated: memory_00001"
```

### 4. 删除内存（DELETE）

**请求：**
```
命令: 0x03
数据: "memory_00001"
```

**响应（成功）：**
```
状态码: 0x00
数据: "Memory freed: memory_00001"
```

### 5. 查询状态（STATUS）

**请求：**
```
命令: 0x05
数据: "" (空)
```

**响应（成功）：**
```
状态码: 0x00
数据: "Memory Pool Status:\nTotal blocks: 2\n\nMemory ID: memory_00001\n  Description: My Data\n  Blocks: 0 - 0\n  Last Modified: 2024-01-15 10:30:45\n\n..."
```

### 6. 心跳检测（PING）

**请求：**
```
命令: 0x06
数据: "" (空)
```

**响应（成功）：**
```
状态码: 0x00
数据: "PONG"
```

## 错误处理

### 常见错误

1. **ERROR_INVALID_PARAM (0x02)**
   - 原因：参数格式错误或为空
   - 示例：ALLOC 时 content 为空

2. **ERROR_NOT_FOUND (0x04)**
   - 原因：Memory ID 不存在
   - 示例：READ/DELETE/UPDATE 时使用了不存在的 Memory ID

3. **ERROR_NO_MEMORY (0x03)**
   - 原因：内存池空间不足
   - 示例：ALLOC 时内存池已满

### 错误响应示例

**请求：** READ "memory_99999"（不存在的 ID）

**响应：**
```
状态码: 0x04
数据: "Memory ID not found or content is empty: memory_99999"
```

## 注意事项

1. **字节序**：长度字段必须使用大端序（网络字节序）
2. **字符串编码**：所有字符串使用 UTF-8 编码
3. **分隔符**：ALLOC 和 UPDATE 使用 `\0` (NULL) 分隔字段
4. **粘包处理**：确保完整接收响应数据（根据长度字段）
5. **超时设置**：建议设置连接和接收超时
6. **错误处理**：检查状态码，处理各种错误情况
7. **连接管理**：及时关闭连接，避免资源泄漏

## 完整实现示例

### Python 参考实现

项目提供了完整的 Python 实现参考：`client/client.py`

该实现包含：
- 完整的协议封装
- 所有操作的 API
- 错误处理
- 上下文管理器支持

可以作为其他语言实现的参考。

## 测试建议

1. **连接测试**：先使用 PING 命令测试连接
2. **基本操作**：按顺序测试 ALLOC → READ → UPDATE → READ → DELETE
3. **错误处理**：测试各种错误情况（无效参数、不存在的 ID 等）
4. **并发测试**：多个客户端同时连接和操作
5. **边界测试**：测试大数据、空数据等边界情况

## 故障排除

### 连接失败

**常见原因和解决方案：**

1. **服务器未运行**
   - 检查服务器程序是否正在运行
   - 确认服务器已成功启动并显示 "TCP Server started on port 8888"

2. **IP 地址和端口错误**
   - 确认服务器显示的 IP 地址和端口
   - 检查客户端使用的 IP 和端口是否匹配

3. **防火墙阻止连接**
   - **Windows 防火墙**：需要在防火墙中允许端口 8888 的入站连接
   - 打开"Windows Defender 防火墙" → "高级设置" → "入站规则" → "新建规则"
   - 选择"端口" → TCP → 特定本地端口：8888 → 允许连接
   - 或者在服务器电脑上临时关闭防火墙测试

4. **网络连接问题**
   - **本地连接（127.0.0.1）**：只能在同一台电脑上访问
   - **局域网连接（192.168.x.x 或 172.x.x.x）**：需要客户端和服务器在同一局域网
   - **公网连接**：需要服务器有公网 IP，并配置路由器端口转发

5. **私有 IP 地址限制**
   - `172.29.57.127` 是私有 IP（172.16.0.0 - 172.31.255.255）
   - 只能在同一个局域网内访问
   - 如果朋友的电脑在不同的网络，无法直接访问
   - **解决方案**：
     - 确保朋友的电脑连接到同一个 Wi-Fi/局域网
     - 或者使用公网 IP + 端口转发
     - 或者使用内网穿透工具（如 ngrok、frp 等）

6. **路由器/网络配置**
   - 某些路由器可能阻止局域网内设备之间的通信
   - 检查路由器是否启用了"AP 隔离"或"客户端隔离"功能
   - 确保两台设备在同一网段

### 连接超时

**可能原因：**
- 服务器未运行或已崩溃
- 防火墙阻止了连接
- 网络不通（不在同一网络）
- IP 地址错误

**解决方案：**
1. 在服务器电脑上测试：`telnet 127.0.0.1 8888` 或使用本地客户端
2. 检查服务器日志，确认是否有错误信息
3. 尝试使用服务器的其他 IP 地址（如果有多个网卡）

### 数据接收不完整

- 确保根据长度字段完整接收数据
- 使用循环接收直到接收完所有数据

### 字节序错误

- 确保使用正确的字节序转换函数
- 验证长度字段的解析是否正确

## 更多信息

- 服务器端文档：`../README.md`
- 技术文档：`../doc.md`
- TCP 协议详细说明：`../server/network/README_TCP.md`
- Python 实现参考：`client.py`
