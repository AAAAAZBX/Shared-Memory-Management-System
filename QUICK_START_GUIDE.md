# 快速开始指南 - 外机访问 Server

## 📚 需要查看的文档清单

如果你想在外机访问 server，需要按以下顺序查看这些文档：

### 1. 本地启动 Server

**查看文档**：`README.md`

**关键章节**：
- [编译与运行](#编译与运行) - 了解如何编译和启动服务器
- [使用方法](#使用方法) - 了解服务器基本使用

**快速步骤**：
```bash
cd server
.\run.bat
```

服务器启动后会显示可访问的 IP 地址和端口（默认 8888）。

---

### 2. 配置外部访问

**查看文档**：`server/network/README_TCP.md`

**关键章节**：
- [外部主机访问配置](#外部主机访问配置) - **必须阅读**
  - [服务器配置](#服务器配置) - 确认服务器已正确配置
  - [防火墙配置](#防火墙配置) - **必须配置**，否则外部无法连接
  - [路由器配置](#路由器网关配置公网访问) - 仅公网访问需要
  - [验证连接](#验证连接) - 验证配置是否正确

**快速配置**：
```powershell
# 以管理员身份运行，配置防火墙
netsh advfirewall firewall add rule name="SMM Server" dir=in action=allow protocol=TCP localport=8888

# 验证端口监听
netstat -an | findstr :8888
# 应该看到: TCP    0.0.0.0:8888   0.0.0.0:0    LISTENING
```

---

### 3. 外机客户端启动和使用

根据你的使用方式，选择以下文档之一：

#### 方式 A：使用 Python 客户端（推荐，最简单）

**查看文档**：`client/README.md`

**关键章节**：
- [快速开始](#快速开始) - 了解如何连接服务器
- [协议规范](#协议规范) - 了解协议格式（可选）
- [Python 客户端示例](#python-客户端示例) - 查看完整示例代码

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

#### 方式 B：使用 C++ SDK（命令行工具）

**查看文档**：`sdk/README.md`

**关键章节**：
- [客户端 SDK 概述](#客户端-sdk-概述) - 了解 SDK 功能
- [快速开始](#快速开始) - 了解如何编译和使用
- [方式一：交互式命令行（推荐）](#方式一交互式命令行推荐) - 类似 server 的命令行

**快速使用**：
```bash
# 编译客户端 CLI
cd sdk
.\build.bat

# 运行客户端（交互式命令行）
.\examples\client_cli.exe 192.168.1.100 8888

# 然后可以像 server 一样输入命令：
client> alloc "测试" "Hello World"
client> read memory_00001
client> status --memory
client> quit
```

#### 方式 C：使用 DLL/SDK（本地调用，无需网络）

**查看文档**：`sdk/README_DLL.md`

**说明**：这种方式是直接链接 DLL，不需要网络连接，但需要将 DLL 部署到客户端机器。

**适用场景**：客户端和服务器在同一台机器，或需要本地高性能调用。

---

### 4. 完整连接流程（可选，深入了解）

**查看文档**：`server/network/README_TCP.md`

**关键章节**：
- [完整连接流程](#完整连接流程) - 了解详细的连接和命令发送过程
- [完整示例](#完整示例外部主机连接并发送命令) - 查看完整的 Python 示例

**适用场景**：需要深入了解协议细节或自己实现客户端。

---

## 📋 文档阅读顺序总结

### 最小化阅读（快速上手）

1. **`README.md`** - 查看"编译与运行"章节，启动服务器
2. **`server/network/README_TCP.md`** - 查看"外部主机访问配置"章节，配置防火墙
3. **`client/README.md`** - 查看"快速开始"和示例代码，使用 Python 客户端

### 完整阅读（深入了解）

1. **`README.md`** - 了解项目整体和服务器使用
2. **`server/network/README_TCP.md`** - 了解协议规范、外部访问配置、完整连接流程
3. **`client/README.md`** 或 **`sdk/README.md`** - 根据选择的客户端方式阅读

---

## 🚀 快速开始步骤

### 步骤 1：启动服务器（本地）

```bash
cd server
.\run.bat
```

记录服务器显示的 IP 地址，例如：`192.168.1.100:8888`

### 步骤 2：配置防火墙（服务器端）

```powershell
# 以管理员身份运行 PowerShell
netsh advfirewall firewall add rule name="SMM Server" dir=in action=allow protocol=TCP localport=8888
```

### 步骤 3：连接客户端（外机）

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

---

## 📖 文档详细说明

| 文档 | 用途 | 必读章节 |
|------|------|---------|
| **README.md** | 项目总览和服务器使用 | 编译与运行、使用方法 |
| **server/network/README_TCP.md** | TCP 协议和外部访问 | 外部主机访问配置（必须） |
| **client/README.md** | Python 客户端使用 | 快速开始、Python 客户端示例 |
| **sdk/README.md** | C++ SDK 使用 | 快速开始、方式一：交互式命令行 |
| **sdk/README_DLL.md** | DLL 方式使用 | 仅当需要本地调用时阅读 |

---

## ⚠️ 重要提示

1. **服务器 IP 地址**：客户端必须使用服务器显示的 IP 地址（如 `192.168.1.100`），**不能使用 `localhost` 或 `127.0.0.1`**
2. **防火墙配置**：**必须配置**，否则外部主机无法连接
3. **公网访问**：需要配置路由器端口转发（见 `server/network/README_TCP.md` 的"路由器配置"章节）
4. **端口**：默认端口是 8888，确保没有被其他程序占用

---

## 🔍 故障排除

如果遇到连接问题，按顺序检查：

1. **服务器是否启动**：查看服务器控制台输出
2. **防火墙是否配置**：`netstat -an | findstr :8888` 应该显示 `0.0.0.0:8888`
3. **IP 地址是否正确**：使用服务器显示的 IP，不是 localhost
4. **网络是否连通**：使用 `ping` 测试网络连通性

详细故障排除请参考：`server/network/README_TCP.md` 的"常见问题"章节。
