# Shared Memory Management SDK

这是 Shared Memory Management 系统的 SDK 发布包。

## 两种使用方式

### 1. DLL/SDK 方式（推荐，本地调用）

**Server 核心功能编译成 DLL**，客户端直接链接使用，无需网络连接。

- 📦 **无需下载客户端工具**
- 🚀 **高性能**：本地调用，无网络延迟
- 🔗 **易于集成**：提供 C API，支持多种语言

详细说明请参考：[README_DLL.md](README_DLL.md)

### 2. TCP 客户端方式（远程访问）

通过 TCP 连接到服务器进行操作（已废弃，推荐使用 DLL 方式）。

## 目录结构

```
sdk/
├── include/          # 头文件
│   └── client_sdk.h # 客户端 SDK 头文件
├── lib/              # 库文件（DLL 和导入库）
├── docs/             # 文档
├── src/              # 源代码
│   └── client_sdk.cpp
└── examples/         # 示例代码
    ├── basic_usage.cpp    # C API 基础使用示例
    └── client_cli.cpp      # 客户端命令行工具示例
```

## 客户端 SDK 概述

客户端 SDK 提供类似 server 端的命令行接口，通过 TCP 连接到服务器进行操作。

### 主要特性

- ✅ **命令行接口**：提供与 server 端相同的命令（alloc, read, update, free, status 等）
- ✅ **交互式 CLI**：支持交互式命令行界面（类似 server 的 REPL）
- ✅ **TCP 连接**：通过 TCP 协议连接到服务器
- ✅ **跨平台**：支持 Windows 和 Linux
- ✅ **易于集成**：可编译成 DLL（Windows）或共享库（Linux）

## 快速开始

### 1. 编译客户端 SDK

#### Windows (MinGW/MSVC)

```bash
# 编译静态库
g++ -std=c++17 -c src/client_sdk.cpp -Iinclude -o lib/client_sdk.o
ar rcs lib/libsmm_client.a lib/client_sdk.o

# 编译 DLL
g++ -std=c++17 -shared src/client_sdk.cpp -Iinclude -o lib/smm_client.dll -lws2_32 -Wl,--out-implib,lib/smm_client.lib

# 编译示例程序
g++ -std=c++17 examples/client_cli.cpp -Iinclude -Llib -lsmm_client -o examples/client_cli.exe -lws2_32
```

#### Linux

```bash
# 编译静态库
g++ -std=c++17 -c src/client_sdk.cpp -Iinclude -fPIC -o lib/client_sdk.o
ar rcs lib/libsmm_client.a lib/client_sdk.o

# 编译共享库
g++ -std=c++17 -shared src/client_sdk.cpp -Iinclude -fPIC -o lib/libsmm_client.so

# 编译示例程序
g++ -std=c++17 examples/client_cli.cpp -Iinclude -Llib -lsmm_client -o examples/client_cli
```

### 2. 使用客户端 SDK

#### 方式一：交互式命令行（推荐）

```cpp
#include "client_sdk.h"

int main() {
    SMMClient::ClientSDK client("127.0.0.1", 8888);
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

#### 方式二：编程接口

```cpp
#include "client_sdk.h"
#include <iostream>

int main() {
    SMMClient::ClientSDK client("127.0.0.1", 8888);
    
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

## API 参考

### ClientSDK 类

#### 构造函数

```cpp
ClientSDK(const std::string& host = "127.0.0.1", uint16_t port = 8888);
```

创建客户端 SDK 实例。

#### 连接管理

```cpp
bool Connect();              // 连接到服务器
void Disconnect();           // 断开连接
bool IsConnected() const;    // 检查是否已连接
```

#### 命令执行

```cpp
bool ExecuteCommand(const std::string& command);
bool ExecuteCommandWithOutput(const std::string& command, std::string& output);
```

执行命令并获取结果。

#### 交互式命令行

```cpp
void StartInteractiveCLI(const std::string& prompt = "client> ");
```

启动交互式命令行界面。

## 支持的命令

客户端 SDK 支持与 server 端相同的命令：

- `alloc "<description>" "<content>"` - 分配内存
- `read <memory_id>` - 读取内存内容
- `update <memory_id> "<new_content>"` - 更新内存内容
- `free <memory_id>` - 释放内存
- `delete <memory_id>` - 删除内存（free 的别名）
- `status [--memory|--block]` - 查询状态
- `help` - 显示帮助信息
- `quit` / `exit` - 退出客户端

## 编译选项

### 编译成 DLL（Windows）

```bash
g++ -std=c++17 -shared src/client_sdk.cpp -Iinclude -o lib/smm_client.dll -lws2_32 -Wl,--out-implib,lib/smm_client.lib -DCLIENT_SDK_EXPORTS
```

### 编译成静态库

```bash
# Windows
g++ -std=c++17 -c src/client_sdk.cpp -Iinclude -o lib/client_sdk.o
ar rcs lib/libsmm_client.a lib/client_sdk.o

# Linux
g++ -std=c++17 -c src/client_sdk.cpp -Iinclude -fPIC -o lib/client_sdk.o
ar rcs lib/libsmm_client.a lib/client_sdk.o
```

### 编译成共享库（Linux）

```bash
g++ -std=c++17 -shared src/client_sdk.cpp -Iinclude -fPIC -o lib/libsmm_client.so
```

## 使用示例

### 示例 1：交互式命令行

```bash
# 编译
g++ -std=c++17 examples/client_cli.cpp -Iinclude -Llib -lsmm_client -o client_cli -lws2_32

# 运行
./client_cli 127.0.0.1 8888
```

### 示例 2：集成到自己的程序

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

## 注意事项

1. **服务器必须先运行**：客户端需要连接到运行中的服务器
2. **网络连接**：确保网络连接正常，防火墙允许连接
3. **编码支持**：客户端支持 UTF-8 编码，可以处理中文
4. **线程安全**：每个 ClientSDK 实例不是线程安全的，多线程使用需要加锁

## 故障排除

### 连接失败

- 检查服务器是否运行
- 检查服务器地址和端口是否正确
- 检查防火墙设置

### 编译错误

- 确保包含正确的头文件路径
- Windows 需要链接 `ws2_32.lib`
- Linux 需要安装开发工具链

## 文档

详细 API 文档请参考：
- `docs/API.md` - 完整 API 参考
- `examples/` - 更多示例代码
