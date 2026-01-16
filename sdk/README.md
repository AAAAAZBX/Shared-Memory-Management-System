# Shared Memory Management SDK

这是 Shared Memory Management 系统的 SDK 发布包。

## 目录

- [Shared Memory Management SDK](#shared-memory-management-sdk)
  - [目录](#目录)
  - [概述](#概述)
  - [TCP 客户端方式（远程访问）](#tcp-客户端方式远程访问)
      - [客户端 SDK 概述](#客户端-sdk-概述)
        - [主要特性](#主要特性)
      - [完整流程：从服务器启动到远程客户端连接](#完整流程从服务器启动到远程客户端连接)
        - [阶段 1：服务器端准备](#阶段-1服务器端准备)
        - [阶段 2：客户端端准备](#阶段-2客户端端准备)
        - [阶段 3：客户端连接和操作](#阶段-3客户端连接和操作)
        - [阶段 4：故障排除](#阶段-4故障排除)
        - [完整示例：局域网访问](#完整示例局域网访问)
        - [完整示例：公网访问（需要路由器配置）](#完整示例公网访问需要路由器配置)
      - [快速开始](#快速开始)
        - [1. 在服务器端编译客户端程序（静态链接版本）](#1-在服务器端编译客户端程序静态链接版本)
        - [2. 部署到客户端机器](#2-部署到客户端机器)
        - [3. 使用客户端 SDK](#3-使用客户端-sdk)
      - [API 参考](#api-参考-1)
        - [ClientSDK 类](#clientsdk-类)
      - [支持的命令](#支持的命令)
      - [使用示例](#使用示例)
      - [注意事项](#注意事项-1)
      - [故障排除](#故障排除-1)
        - [问题：客户端显示 "Please install MSYS2/MinGW-w64"](#问题客户端显示-please-install-msys2mingw-w64)
        - [连接失败](#连接失败)
        - [编译错误](#编译错误)
  - [目录结构](#目录结构)

---

## 概述

Shared Memory Management 系统提供 TCP 客户端 SDK，支持远程客户端通过网络连接到服务器进行操作。

---

## TCP 客户端方式（远程访问）

通过 TCP 连接到服务器进行操作，支持局域网和公网访问。

#### 客户端 SDK 概述

客户端 SDK 提供类似 server 端的命令行接口，通过 TCP 连接到服务器进行操作。

##### 主要特性

- ✅ **命令行接口**：提供与 server 端相同的命令（alloc, read, update, free, status 等）
- ✅ **交互式 CLI**：支持交互式命令行界面（类似 server 的 REPL）
- ✅ **TCP 连接**：通过 TCP 协议连接到服务器
- ✅ **Windows 平台**：支持 Windows 平台
- ✅ **易于集成**：提供 C++ SDK，支持静态链接
- ✅ **远程访问**：支持局域网和公网访问

#### 完整流程：从服务器启动到远程客户端连接

本流程将指导你完成从服务器端启动到远程客户端成功连接的完整过程。

##### 阶段 1：服务器端准备

**步骤 1.1：启动服务器**

在服务器机器上：

```bash
# 进入服务器目录
cd server

# 编译并运行服务器（Windows PowerShell）
cmd /c run.bat

# 或使用 PowerShell 脚本（如果存在）
.\run.ps1
```

**步骤 1.2：记录服务器 IP 地址**

服务器启动后会显示可访问的 IP 地址，例如：

```
##############################################
#                                            #
#            Shared Memory Manager           #
#                                            #
##############################################

Server is ready for client connections:

  Private IP (LAN access):
    192.168.20.31:8888 (LAN access)
    172.29.56.108:8888 (LAN access)

Note: External clients should use Private IP addresses.
========================================
```

**重要提示**：
- ✅ **使用主网卡 IP**（如 `192.168.20.31`）- 这是外机可以 ping 通的 IP
- ❌ **不要使用虚拟网卡 IP**（如 `172.29.56.108`）- 外机无法访问

**步骤 1.3：配置防火墙（服务器端）**

在服务器上以**管理员身份**运行 PowerShell：

```powershell
# 允许 TCP 8888 端口入站连接
netsh advfirewall firewall add rule name="SMM Server" dir=in action=allow protocol=TCP localport=8888

# 验证规则是否添加成功
netsh advfirewall firewall show rule name="SMM Server"
```

**步骤 1.4：验证服务器监听状态**

在服务器上运行：

```powershell
# 检查端口是否监听
netstat -an | findstr :8888
```

应该看到：
```
TCP    0.0.0.0:8888           0.0.0.0:0              LISTENING
```

`0.0.0.0:8888` 表示服务器监听所有网络接口的 8888 端口，可以通过任何 IP 访问。

##### 阶段 2：客户端端准备

**步骤 2.1：在服务器端编译客户端程序**

**⚠️ 重要提示**：
- ✅ 这些编译命令**必须在服务器端**（有开发环境的机器）运行
- ❌ **不要在客户端机器上运行这些编译命令**
- ✅ 客户端机器**不需要安装任何开发工具**（MinGW-w64、MSYS2 等）
- ✅ 客户端只需要复制编译好的 `.exe` 文件

**在服务器端（有开发环境的机器）编译静态链接版本**：

```bash
# 在服务器端执行（不是客户端！）
cd sdk

# 编译静态链接版本（独立可执行文件，无需 DLL）
cmd /c build_client_cli_static.bat
# 输出：examples/client_cli_static.exe

# 或编译简单客户端
cmd /c build_client_static.bat
# 输出：client_static.exe
```

**如果看到 "Please install MinGW-w64" 错误**：
- 说明你在客户端机器上运行了编译脚本
- 解决：在**服务器端**运行编译脚本，然后将编译好的 `.exe` 文件复制到客户端

**部署到客户端机器**：

有两种方式将编译好的文件提供给客户端：

**方式一：通过 Git（推荐）**：
- `*_static.exe` 文件已配置为可提交到 Git（`.gitignore` 中已排除）
- 在服务器端编译后，提交到 GitHub
- 客户端直接从 GitHub 拉取即可使用

**方式二：直接复制**：
- 将 `client_cli_static.exe` 或 `client_static.exe` 直接复制到客户端机器

**客户端无需安装任何开发工具或 DLL**，直接运行即可。

**重要提示**：
- ✅ 静态链接版本（`*_static.exe`）是独立可执行文件，不依赖任何 DLL
- ✅ 客户端机器**不需要**安装 MinGW-w64、MSYS2 或任何其他开发工具
- ✅ 客户端机器**不需要**任何 DLL 文件
- ⚠️ 如果通过 Git 复制代码，`.gitignore` 会忽略编译文件（`*.exe`, `*.dll`），必须在服务器端编译后再复制可执行文件

**步骤 2.2：测试网络连通性（可选但推荐）**

在客户端机器上测试能否连接到服务器：

```bash
# 方法 1：使用 ping（测试 ICMP）
ping 192.168.20.31

# 方法 2：使用 telnet（测试 TCP，推荐）
telnet 192.168.20.31 8888

# 方法 3：使用 PowerShell（Windows）
Test-NetConnection -ComputerName 192.168.20.31 -Port 8888

# 方法 4：使用 Python 快速测试
python -c "import socket; s = socket.socket(); s.settimeout(3); result = s.connect_ex(('192.168.20.31', 8888)); print('Connected!' if result == 0 else 'Failed'); s.close()"
```

**预期结果**：
- ✅ 如果连接成功，说明网络和防火墙配置正确
- ❌ 如果连接失败，检查：
  1. 服务器是否正在运行
  2. 防火墙是否已配置
  3. IP 地址是否正确（使用主网卡 IP）

##### 阶段 3：客户端连接和操作

**步骤 3.1：运行客户端程序**

在客户端机器上：

```bash
# 方式一：使用静态链接版本（推荐，无需 DLL）
.\client_cli_static.exe 192.168.20.31 8888
# 或
.\client_static.exe 192.168.20.31 8888

# 方式二：使用动态链接版本（需要 MinGW 运行时 DLL）
.\examples\client_cli.exe 192.168.20.31 8888
# 或
.\client.exe 192.168.20.31 8888
```

**注意**：如果使用动态链接版本，确保以下文件在同一目录：
- 可执行文件（`client_cli.exe` 或 `client.exe`）
- MinGW 运行时 DLL（`libgcc_s_seh-1.dll`, `libstdc++-6.dll` 等）

**步骤 3.2：使用客户端**

连接成功后，会显示：

```
##############################################
#                                            #
#        Shared Memory Manager Client        #
#                                            #
##############################################

Connected to server at 192.168.20.31:8888
Type 'help' for help, 'quit' or 'exit' to exit.

client> 
```

**步骤 3.3：执行命令**

客户端支持与服务器端相同的命令：

```bash
# 分配内存
client> alloc "测试数据" "Hello World"
Memory allocated: memory_00001

# 读取内存
client> read memory_00001
----------------------------------------
Memory ID: memory_00001
Description: 测试数据
Last Modified: 2026-01-15 10:30:45
----------------------------------------
Hello World

# 查看状态
client> status --memory
Memory Pool Status:
| MemoryID     | Description | Bytes | Range                 | Last Modified       |
| ------------ | ----------- | ----- | --------------------- | ------------------- |
| memory_00001 | 测试数据    | 11    | block_000 - block_000 | 2026-01-15 10:30:45 |

# 更新内存
client> update memory_00001 "Updated Content"

# 释放内存
client> free memory_00001

# 退出
client> quit
Bye!
```

##### 阶段 4：故障排除

如果连接失败，按以下步骤排查：

**问题 1：连接超时或拒绝连接**

```bash
# 检查服务器是否运行
# 在服务器上查看控制台输出

# 检查防火墙配置
netsh advfirewall firewall show rule name="SMM Server"

# 检查端口监听
netstat -an | findstr :8888
```

**问题 2：使用了错误的 IP 地址**

- ✅ 使用服务器显示的主网卡 IP（外机可以 ping 通的 IP）
- ❌ 不要使用虚拟网卡 IP

**问题 3：防火墙阻止连接**

```powershell
# 重新配置防火墙（以管理员身份运行）
netsh advfirewall firewall delete rule name="SMM Server"
netsh advfirewall firewall add rule name="SMM Server" dir=in action=allow protocol=TCP localport=8888
```

**问题 4：网络不通**

```bash
# 测试网络连通性
ping <服务器IP>

# 如果 ping 不通，检查：
# 1. 服务器和客户端是否在同一局域网
# 2. 路由器/交换机配置是否正确
# 3. 网络接口是否正常
```

##### 完整示例：局域网访问

**服务器端（192.168.20.31）**：

```bash
# 1. 启动服务器
cd server
cmd /c run.bat

# 2. 配置防火墙（管理员权限）
netsh advfirewall firewall add rule name="SMM Server" dir=in action=allow protocol=TCP localport=8888
```

**客户端端（192.168.20.32，同一局域网）**：

```bash
# 1. 编译客户端
cd sdk
cmd /c build.bat

# 2. 测试连接
Test-NetConnection -ComputerName 192.168.20.31 -Port 8888

# 3. 运行客户端
.\examples\client_cli.exe 192.168.20.31 8888

# 4. 使用客户端
client> alloc "测试" "Hello from remote client"
client> status --memory
client> quit
```

##### 完整示例：公网访问（需要路由器配置）

**服务器端**：

```bash
# 1. 启动服务器（同上）
cd server
cmd /c run.bat

# 2. 配置防火墙（同上）
netsh advfirewall firewall add rule name="SMM Server" dir=in action=allow protocol=TCP localport=8888

# 3. 配置路由器端口转发
# - 登录路由器管理界面
# - 添加端口转发规则：
#   外部端口：8888
#   内部 IP：192.168.20.31（服务器内网 IP）
#   内部端口：8888
#   协议：TCP

# 4. 获取公网 IP
# 访问 https://www.whatismyip.com/ 查看公网 IP
```

**客户端端（互联网上的任意机器）**：

```bash
# 1. 在服务器端编译静态链接版本（见步骤 2.1）
# 2. 将 client_cli_static.exe 复制到客户端机器
# 3. 在客户端机器上运行（无需任何开发工具）
.\client_cli_static.exe <公网IP> 8888
```

#### 快速开始

**⚠️ 重要提示**：
- ✅ 以下编译步骤**必须在服务器端**（有开发环境的机器）完成
- ❌ **不要在客户端机器上运行编译命令**
- ✅ 客户端机器**不需要安装任何开发工具**（MinGW-w64、MSYS2 等）
- ✅ 客户端只需要复制编译好的 `.exe` 文件

**如果看到 "Please install MinGW-w64" 错误**：
- 说明你在客户端机器上运行了编译脚本
- 解决：在**服务器端**运行编译脚本，然后将编译好的 `.exe` 文件复制到客户端

##### 1. 在服务器端编译客户端程序（静态链接版本）

**编译 `include/client.cpp`（简单示例）**：

```bash
# 在服务器端执行
cd sdk
cmd /c build_client_static.bat
# 输出：client_static.exe（独立可执行文件，无需 DLL）
```

**编译 `examples/client_cli.cpp`（完整示例）**：

```bash
# 在服务器端执行
cd sdk
cmd /c build_client_cli_static.bat
# 输出：examples/client_cli_static.exe（独立可执行文件，无需 DLL）
```

##### 2. 部署到客户端机器

**只需复制可执行文件**：

将 `client_static.exe` 或 `client_cli_static.exe` 复制到客户端机器即可运行。

**客户端机器要求**：
- ✅ Windows 操作系统
- ✅ 无需安装任何开发工具（MinGW-w64、MSYS2 等）
- ✅ 无需任何 DLL 文件
- ✅ 无需配置 PATH 环境变量

**在客户端机器上运行**：

```bash
# 直接运行，无需任何依赖
.\client_cli_static.exe 192.168.1.100 8888
# 或
.\client_static.exe 192.168.1.100 8888
```

##### 3. 使用客户端 SDK

**方式一：交互式命令行（推荐）**：

```cpp
#include "client_sdk.h"

int main() {
    SMMClient::ClientSDK client("192.168.1.100", 8888);
    client.StartInteractiveCLI();  // 启动交互式命令行
    return 0;
}
```

运行后可以像 server 端一样输入命令：
```
client> alloc "测试数据" "Hello World"
client> read memory_00001
client> status --memory
client> quit
```

**运行编译好的程序**：

```bash
# 运行 client_cli.exe（需要指定服务器地址和端口）
.\examples\client_cli.exe 192.168.1.100 8888

# 或运行 client_static.exe（静态链接版本）
.\client_static.exe 192.168.1.100 8888
```

**方式二：编程接口**：

```cpp
#include "client_sdk.h"
#include <iostream>

int main() {
    SMMClient::ClientSDK client("192.168.1.100", 8888);
    
    if (!client.Connect()) {
        std::cerr << "Failed to connect\n";
        return 1;
    }
    
    // 执行命令
    client.ExecuteCommand("alloc \"My Data\" \"Hello World\"");
    
    std::string output;
    if (client.ExecuteCommandWithOutput("read memory_00001", output)) {
        std::cout << "Read result: " << output << "\n";
    }
    
    client.Disconnect();
    return 0;
}
```

#### API 参考

##### ClientSDK 类

**构造函数**：

```cpp
ClientSDK(const std::string& host, uint16_t port = 8888);
```

创建客户端 SDK 实例。`host` 参数指定服务器 IP 地址（如 `"192.168.1.100"`）。

**连接管理**：

```cpp
bool Connect();              // 连接到服务器
void Disconnect();           // 断开连接
bool IsConnected() const;    // 检查是否已连接
```

**命令执行**：

```cpp
bool ExecuteCommand(const std::string& command);
bool ExecuteCommandWithOutput(const std::string& command, std::string& output);
```

执行命令并获取结果。

**交互式命令行**：

```cpp
void StartInteractiveCLI(const std::string& prompt = "client> ");
```

启动交互式命令行界面。

#### 支持的命令

客户端 SDK 支持与 server 端相同的命令：

- `alloc "<description>" "<content>"` - 分配内存
- `read <memory_id>` - 读取内存内容
- `update <memory_id> "<new_content>"` - 更新内存内容
- `free <memory_id>` - 释放内存
- `delete <memory_id>` - 删除内存（free 的别名）
- `status [--memory|--block]` - 查询状态
- `help` - 显示帮助信息
- `quit` / `exit` - 退出客户端

#### 使用示例

**示例 1：交互式命令行**：

```bash
# 编译
g++ -std=c++17 examples/client_cli.cpp -Iinclude -Llib -lsmm_client -o client_cli -lws2_32

# 运行（指定服务器 IP 地址）
./client_cli 192.168.1.100 8888
```

**示例 2：集成到自己的程序**：

```cpp
#include "client_sdk.h"

void MyApplication() {
    SMMClient::ClientSDK client("192.168.1.100", 8888);
    
    if (client.Connect()) {
        // 分配内存
        client.ExecuteCommand("alloc \"App Data\" \"Some content\"");
        
        // 读取内存
        std::string result;
        client.ExecuteCommandWithOutput("read memory_00001", result);
        
        client.Disconnect();
    }
}
```

#### 注意事项

1. **服务器必须先运行**：客户端需要连接到运行中的服务器
2. **网络连接**：确保网络连接正常，防火墙允许连接
3. **编码支持**：客户端支持 UTF-8 编码，可以处理中文
4. **线程安全**：每个 ClientSDK 实例不是线程安全的，多线程使用需要加锁

#### 故障排除

##### 问题：客户端显示 "Please install MSYS2/MinGW-w64"

**问题原因**：

1. **动态链接版本需要运行时 DLL**：
   - `client_cli.exe` 和 `client.exe` 是动态链接版本
   - 需要 MinGW 运行时 DLL：`libgcc_s_seh-1.dll`, `libstdc++-6.dll` 等
   - 这些 DLL 不在客户端机器上，导致运行时错误

2. **.gitignore 忽略了编译文件**：
   - `.gitignore` 中忽略了 `*.exe`, `*.dll`, `*.a`, `*.lib`
   - 如果通过 Git 复制代码，编译生成的文件不会存在

**解决方案**：

**方案一：使用静态链接版本（推荐）✅**

在服务器端（有开发环境的机器）编译静态链接版本：

```bash
cd sdk

# 编译静态链接版本（独立可执行文件）
cmd /c build_client_cli_static.bat
# 输出：examples/client_cli_static.exe

# 或编译简单客户端
cmd /c build_client_static.bat
# 输出：client_static.exe
```

部署到客户端：
1. 将 `client_cli_static.exe` 或 `client_static.exe` 复制到客户端机器
2. 客户端直接运行，**无需任何 DLL 或开发工具**

```bash
# 在客户端机器上运行
.\client_cli_static.exe 192.168.1.100 8888
```

**方案二：复制运行时 DLL（不推荐）**

如果必须使用动态链接版本，需要复制以下文件到客户端：

1. **可执行文件**：
   - `examples/client_cli.exe` 或 `client.exe`

2. **MinGW 运行时 DLL**（从 MinGW 安装目录的 `bin` 文件夹复制）：
   - `libgcc_s_seh-1.dll`（或 `libgcc_s_dw2-1.dll`，取决于 MinGW 版本）
   - `libstdc++-6.dll`
   - `libwinpthread-1.dll`（如果使用 pthread）

3. **客户端 SDK DLL**（如果使用 DLL 版本）：
   - `lib/smm_client.dll`

**查找 MinGW DLL 的位置**：

```bash
# 在服务器端（有 MinGW 的机器）
where g++
# 输出类似：D:\software\msys2\ucrt64\bin\g++.exe

# DLL 在同一个 bin 目录中
# 复制以下文件到客户端（与 exe 同一目录）：
# D:\software\msys2\ucrt64\bin\libgcc_s_seh-1.dll
# D:\software\msys2\ucrt64\bin\libstdc++-6.dll
# D:\software\msys2\ucrt64\bin\libwinpthread-1.dll
```

**推荐工作流程**：

**服务器端（开发机器）**：
```bash
# 1. 编译静态链接版本
cd sdk
cmd /c build_client_cli_static.bat

# 2. 将编译好的文件复制到客户端
# 只需复制：examples/client_cli_static.exe
```

**客户端（目标机器）**：
```bash
# 直接运行，无需任何依赖
.\client_cli_static.exe 192.168.1.100 8888
```

**验证部署**：

在客户端机器上运行：
```bash
# 检查文件是否存在
dir client_cli_static.exe

# 运行程序
.\client_cli_static.exe 192.168.1.100 8888
```

如果仍然提示需要 MinGW，检查：
1. 是否使用了静态链接版本（`*_static.exe`）
2. 文件是否完整复制
3. 是否有其他依赖问题

**总结**：
- ✅ **推荐**：使用静态链接版本（`*_static.exe`），只需复制一个文件
- ❌ **不推荐**：使用动态链接版本，需要复制多个 DLL 文件

##### 连接失败

- 检查服务器是否运行
- 检查服务器地址和端口是否正确
- 检查防火墙设置

##### 编译错误

- 确保包含正确的头文件路径
- Windows 需要链接 `ws2_32.lib`

---

## 目录结构

```
sdk/
├── include/          # 头文件
│   └── client_sdk.h # 客户端 SDK 头文件
├── lib/              # 库文件
│   └── libsmm_client.a # 客户端静态库
├── docs/             # 文档
├── src/              # 源代码
│   └── client_sdk.cpp # 客户端 SDK 实现
└── examples/         # 示例代码
    └── client_cli.cpp # 客户端命令行工具示例
```

**注意**：
- 客户端 SDK 头文件在 `sdk/include/client_sdk.h`
