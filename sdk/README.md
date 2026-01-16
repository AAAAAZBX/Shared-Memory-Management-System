# Shared Memory Management SDK

这是 Shared Memory Management 系统的 SDK 发布包。

## 目录

- [概述](#概述)
- [两种使用方式](#两种使用方式)
  - [方式一：DLL/SDK 方式（推荐，本地调用）](#方式一dllsdk-方式推荐本地调用)
    - [架构说明](#架构说明)
    - [编译 DLL](#编译-dll)
    - [使用方式](#使用方式)
    - [API 参考](#api-参考)
    - [完整示例](#完整示例)
    - [与其他语言的集成](#与其他语言的集成)
    - [注意事项](#注意事项)
    - [与 TCP 方式的区别](#与-tcp-方式的区别)
    - [故障排除](#故障排除)
  - [方式二：TCP 客户端方式（远程访问）](#方式二tcp-客户端方式远程访问)
    - [客户端 SDK 概述](#客户端-sdk-概述)
    - [快速开始](#快速开始)
    - [API 参考](#api-参考-1)
    - [支持的命令](#支持的命令)
    - [使用示例](#使用示例)
    - [注意事项](#注意事项-1)
    - [故障排除](#故障排除-1)
- [目录结构](#目录结构)

---

## 概述

Shared Memory Management 系统提供两种 SDK 使用方式：

1. **DLL/SDK 方式（推荐）**：Server 核心功能编译成 DLL，客户端直接链接使用，无需网络连接
2. **TCP 客户端方式**：通过 TCP 连接到服务器进行操作（已废弃，推荐使用 DLL 方式）

## 两种使用方式

### 方式一：DLL/SDK 方式（推荐，本地调用）

**Server 核心功能编译成 DLL**，客户端直接链接使用，无需网络连接。

- 📦 **无需下载客户端工具**
- 🚀 **高性能**：本地调用，无网络延迟
- 🔗 **易于集成**：提供 C API，支持多种语言

#### 架构说明

```
┌─────────────────┐
│  客户端程序      │
│  (你的应用)      │
└────────┬────────┘
         │ 直接链接 DLL
         ▼
┌─────────────────┐
│   smm.dll        │  ← Server 核心功能（DLL）
│  (共享内存管理)  │
└─────────────────┘
```

**特点**：
- ✅ **本地调用**：客户端直接调用 DLL，无需网络
- ✅ **无需下载**：客户端只需链接 DLL，不需要单独的工具
- ✅ **高性能**：本地调用，无网络延迟
- ✅ **易于集成**：提供 C API，支持多种语言

#### 编译 DLL

**Windows**：

```bash
cd core
build_dll.bat
```

输出文件：
- `sdk/lib/smm.dll` - 动态链接库
- `sdk/lib/smm.lib` - 导入库（用于链接）
- `sdk/lib/libsmm.a` - 静态库
- `sdk/include/smm_api.h` - 头文件（实际在 `core/api/smm_api.h`）

**Linux**：

```bash
cd core
g++ -std=c++17 -shared -fPIC -DSMM_BUILDING_DLL \
    -Iapi -Ishared_memory_pool -Ipersistence \
    api/smm_api.cpp \
    shared_memory_pool/shared_memory_pool.cpp \
    persistence/persistence.cpp \
    -o ../sdk/lib/libsmm.so
```

#### 使用方式

##### 方式一：动态链接（推荐）

**1. 包含头文件**：

```cpp
#include "smm_api.h"
```

**2. 链接 DLL**：

**编译时**：
```bash
g++ -std=c++17 your_program.cpp -I../sdk/include -L../sdk/lib -lsmm -o your_program.exe
```

**运行时**：
- 确保 `smm.dll` 在可执行文件目录或系统 PATH 中

**3. 使用 API**：

```cpp
// 创建内存池
SMM_PoolHandle pool = smm_create_pool(1024 * 1024 * 1024);

// 分配内存
char memory_id[64];
smm_alloc(pool, "描述", "内容", strlen("内容"), memory_id, sizeof(memory_id));

// 读取内存
char buffer[256];
size_t actual_size;
smm_read(pool, memory_id, buffer, sizeof(buffer), &actual_size);

// 释放内存
smm_free(pool, memory_id);

// 销毁内存池
smm_destroy_pool(pool);
```

##### 方式二：静态链接

```bash
g++ -std=c++17 your_program.cpp -I../sdk/include -L../sdk/lib -lsmm -static -o your_program.exe
```

静态链接后，不需要单独的 DLL 文件。

#### API 参考

##### 生命周期管理

```cpp
SMM_PoolHandle smm_create_pool(size_t pool_size);
SMM_ErrorCode smm_destroy_pool(SMM_PoolHandle pool);
SMM_ErrorCode smm_reset_pool(SMM_PoolHandle pool);
```

##### 内存操作

```cpp
SMM_ErrorCode smm_alloc(
    SMM_PoolHandle pool,
    const char* description,
    const void* data,
    size_t data_size,
    char* memory_id_out,
    size_t memory_id_size
);

SMM_ErrorCode smm_free(SMM_PoolHandle pool, const char* memory_id);

SMM_ErrorCode smm_update(
    SMM_PoolHandle pool,
    const char* memory_id,
    const void* new_data,
    size_t new_data_size
);

SMM_ErrorCode smm_read(
    SMM_PoolHandle pool,
    const char* memory_id,
    void* buffer,
    size_t buffer_size,
    size_t* actual_size
);
```

##### 查询操作

```cpp
SMM_ErrorCode smm_get_status(SMM_PoolHandle pool, SMM_StatusInfo* status_out);
SMM_ErrorCode smm_get_memory_info(SMM_PoolHandle pool, const char* memory_id, SMM_MemoryInfo* info_out);
```

##### 其他操作

```cpp
SMM_ErrorCode smm_compact(SMM_PoolHandle pool);
SMM_ErrorCode smm_save(SMM_PoolHandle pool, const char* filename);
SMM_ErrorCode smm_load(SMM_PoolHandle pool, const char* filename);
```

#### 完整示例

参考 `sdk/examples/use_dll.cpp`：

```bash
# 编译示例
g++ -std=c++17 sdk/examples/use_dll.cpp -Isdk/include -Lsdk/lib -lsmm -o use_dll.exe

# 运行（确保 smm.dll 在 PATH 中）
./use_dll.exe
```

#### 与其他语言的集成

##### Python (使用 ctypes)

```python
import ctypes

# 加载 DLL
smm = ctypes.CDLL('smm.dll')

# 定义函数签名
smm.smm_create_pool.argtypes = [ctypes.c_size_t]
smm.smm_create_pool.restype = ctypes.c_void_p

# 使用
pool = smm.smm_create_pool(1024 * 1024 * 1024)
```

##### C# (P/Invoke)

```csharp
using System;
using System.Runtime.InteropServices;

public class SMM {
    [DllImport("smm.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr smm_create_pool(ulong pool_size);
    
    // 使用
    IntPtr pool = smm_create_pool(1024 * 1024 * 1024);
}
```

#### 注意事项

1. **线程安全**：DLL 内部使用互斥锁，支持多线程调用
2. **内存管理**：使用完毕后必须调用 `smm_destroy_pool()` 释放资源
3. **错误处理**：检查返回值，使用 `smm_get_error_string()` 获取错误信息
4. **持久化**：使用 `smm_save()` 和 `smm_load()` 保存和加载内存池状态

#### 与 TCP 方式的区别

| 特性 | DLL/SDK 方式 | TCP 方式 |
|------|-------------|---------|
| **连接方式** | 本地链接 | 网络连接 |
| **性能** | 高（本地调用） | 中等（网络延迟） |
| **部署** | 需要 DLL | 需要服务器运行 |
| **适用场景** | 本地应用 | 远程访问、多客户端 |

#### 故障排除

##### DLL 加载失败

- 确保 `smm.dll` 在可执行文件目录或系统 PATH 中
- 检查 DLL 依赖（使用 Dependency Walker）

##### 链接错误

- 确保包含正确的头文件路径 `-Isdk/include`
- 确保链接库路径正确 `-Lsdk/lib`
- Windows 可能需要链接额外的运行时库

##### 运行时错误

- 检查返回值，使用 `smm_get_last_error()` 获取错误码
- 确保内存池已正确初始化

---

### 方式二：TCP 客户端方式（远程访问）

通过 TCP 连接到服务器进行操作（已废弃，推荐使用 DLL 方式）。

#### 客户端 SDK 概述

客户端 SDK 提供类似 server 端的命令行接口，通过 TCP 连接到服务器进行操作。

##### 主要特性

- ✅ **命令行接口**：提供与 server 端相同的命令（alloc, read, update, free, status 等）
- ✅ **交互式 CLI**：支持交互式命令行界面（类似 server 的 REPL）
- ✅ **TCP 连接**：通过 TCP 协议连接到服务器
- ✅ **跨平台**：支持 Windows 和 Linux
- ✅ **易于集成**：可编译成 DLL（Windows）或共享库（Linux）

#### 快速开始

##### 1. 编译客户端 SDK

**Windows (MinGW/MSVC)**：

```bash
# 编译静态库
g++ -std=c++17 -c src/client_sdk.cpp -Iinclude -o lib/client_sdk.o
ar rcs lib/libsmm_client.a lib/client_sdk.o

# 编译 DLL
g++ -std=c++17 -shared src/client_sdk.cpp -Iinclude -o lib/smm_client.dll -lws2_32 -Wl,--out-implib,lib/smm_client.lib

# 编译示例程序
g++ -std=c++17 examples/client_cli.cpp -Iinclude -Llib -lsmm_client -o examples/client_cli.exe -lws2_32
```

**Linux**：

```bash
# 编译静态库
g++ -std=c++17 -c src/client_sdk.cpp -Iinclude -fPIC -o lib/client_sdk.o
ar rcs lib/libsmm_client.a lib/client_sdk.o

# 编译共享库
g++ -std=c++17 -shared src/client_sdk.cpp -Iinclude -fPIC -o lib/libsmm_client.so

# 编译示例程序
g++ -std=c++17 examples/client_cli.cpp -Iinclude -Llib -lsmm_client -o examples/client_cli
```

##### 2. 使用客户端 SDK

**方式一：交互式命令行（推荐）**：

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

**方式二：编程接口**：

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

#### API 参考

##### ClientSDK 类

**构造函数**：

```cpp
ClientSDK(const std::string& host = "127.0.0.1", uint16_t port = 8888);
```

创建客户端 SDK 实例。

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

# 运行
./client_cli 127.0.0.1 8888
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

##### 连接失败

- 检查服务器是否运行
- 检查服务器地址和端口是否正确
- 检查防火墙设置

##### 编译错误

- 确保包含正确的头文件路径
- Windows 需要链接 `ws2_32.lib`
- Linux 需要安装开发工具链

---

## 目录结构

```
sdk/
├── include/          # 头文件
│   └── client_sdk.h # 客户端 SDK 头文件（TCP 方式）
├── lib/              # 库文件（DLL 和导入库）
│   ├── smm.dll       # Server 核心 DLL（DLL 方式）
│   ├── smm.lib       # 导入库（DLL 方式）
│   └── libsmm_client.a # 客户端静态库（TCP 方式）
├── docs/             # 文档
├── src/              # 源代码
│   └── client_sdk.cpp # 客户端 SDK 实现（TCP 方式）
└── examples/         # 示例代码
    ├── basic_usage.cpp    # C API 基础使用示例（DLL 方式）
    ├── client_cli.cpp     # 客户端命令行工具示例（TCP 方式）
    └── use_dll.cpp        # DLL 使用示例（DLL 方式）
```

**注意**：
- DLL 方式的头文件在 `core/api/smm_api.h`
- TCP 方式的头文件在 `sdk/include/client_sdk.h`
