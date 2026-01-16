# Shared Memory Management DLL/SDK 使用指南

## 概述

Shared Memory Management 系统的核心功能已编译成 DLL/SDK，客户端程序可以直接链接使用，**无需网络连接**，也**无需下载客户端工具**。

## 架构说明

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

## 编译 DLL

### Windows

```bash
cd core
build_dll.bat
```

输出文件：
- `sdk/lib/smm.dll` - 动态链接库
- `sdk/lib/smm.lib` - 导入库（用于链接）
- `sdk/lib/libsmm.a` - 静态库
- `sdk/include/smm_api.h` - 头文件

### Linux

```bash
cd core
g++ -std=c++17 -shared -fPIC -DSMM_BUILDING_DLL \
    -Iapi -Ishared_memory_pool -Ipersistence \
    api/smm_api.cpp \
    shared_memory_pool/shared_memory_pool.cpp \
    persistence/persistence.cpp \
    -o ../sdk/lib/libsmm.so
```

## 使用方式

### 方式一：动态链接（推荐）

#### 1. 包含头文件

```cpp
#include "smm_api.h"
```

#### 2. 链接 DLL

**编译时**：
```bash
g++ -std=c++17 your_program.cpp -I../sdk/include -L../sdk/lib -lsmm -o your_program.exe
```

**运行时**：
- 确保 `smm.dll` 在可执行文件目录或系统 PATH 中

#### 3. 使用 API

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

### 方式二：静态链接

```bash
g++ -std=c++17 your_program.cpp -I../sdk/include -L../sdk/lib -lsmm -static -o your_program.exe
```

静态链接后，不需要单独的 DLL 文件。

## API 参考

### 生命周期管理

```cpp
SMM_PoolHandle smm_create_pool(size_t pool_size);
SMM_ErrorCode smm_destroy_pool(SMM_PoolHandle pool);
SMM_ErrorCode smm_reset_pool(SMM_PoolHandle pool);
```

### 内存操作

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

### 查询操作

```cpp
SMM_ErrorCode smm_get_status(SMM_PoolHandle pool, SMM_StatusInfo* status_out);
SMM_ErrorCode smm_get_memory_info(SMM_PoolHandle pool, const char* memory_id, SMM_MemoryInfo* info_out);
```

### 其他操作

```cpp
SMM_ErrorCode smm_compact(SMM_PoolHandle pool);
SMM_ErrorCode smm_save(SMM_PoolHandle pool, const char* filename);
SMM_ErrorCode smm_load(SMM_PoolHandle pool, const char* filename);
```

## 完整示例

参考 `sdk/examples/use_dll.cpp`：

```bash
# 编译示例
g++ -std=c++17 sdk/examples/use_dll.cpp -Isdk/include -Lsdk/lib -lsmm -o use_dll.exe

# 运行（确保 smm.dll 在 PATH 中）
./use_dll.exe
```

## 与其他语言的集成

### Python (使用 ctypes)

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

### C# (P/Invoke)

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

## 注意事项

1. **线程安全**：DLL 内部使用互斥锁，支持多线程调用
2. **内存管理**：使用完毕后必须调用 `smm_destroy_pool()` 释放资源
3. **错误处理**：检查返回值，使用 `smm_get_error_string()` 获取错误信息
4. **持久化**：使用 `smm_save()` 和 `smm_load()` 保存和加载内存池状态

## 与 TCP 方式的区别

| 特性 | DLL/SDK 方式 | TCP 方式 |
|------|-------------|---------|
| **连接方式** | 本地链接 | 网络连接 |
| **性能** | 高（本地调用） | 中等（网络延迟） |
| **部署** | 需要 DLL | 需要服务器运行 |
| **适用场景** | 本地应用 | 远程访问、多客户端 |

## 故障排除

### DLL 加载失败

- 确保 `smm.dll` 在可执行文件目录或系统 PATH 中
- 检查 DLL 依赖（使用 Dependency Walker）

### 链接错误

- 确保包含正确的头文件路径 `-Isdk/include`
- 确保链接库路径正确 `-Lsdk/lib`
- Windows 可能需要链接额外的运行时库

### 运行时错误

- 检查返回值，使用 `smm_get_last_error()` 获取错误码
- 确保内存池已正确初始化
