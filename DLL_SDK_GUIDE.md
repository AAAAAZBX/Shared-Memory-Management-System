# DLL/SDK 打包指南

## 一、什么是 DLL 和 SDK

### 1.1 DLL（Dynamic Link Library，动态链接库）

**DLL 是什么**：
- 一个包含可执行代码的二进制文件（`.dll`）
- 可以被多个程序共享使用
- 程序运行时动态加载，而不是编译时链接

**类比理解**：
- 就像一本书的"工具库"，多个程序可以"借阅"使用
- 不需要把代码复制到每个程序中
- 更新 DLL 时，所有使用它的程序都能受益

**文件组成**：
```
smm.dll          # 动态链接库（运行时加载）
smm.lib          # 导入库（编译时链接，告诉编译器如何调用 DLL）
smm.h            # 头文件（函数声明）
```

### 1.2 SDK（Software Development Kit，软件开发工具包）

**SDK 是什么**：
- 一套完整的开发工具包，包含：
  - **头文件**（`.h`）：API 声明
  - **库文件**（`.dll` + `.lib`）：可执行代码
  - **文档**：使用说明、API 参考
  - **示例代码**：演示如何使用
  - **工具**：辅助开发的工具

**SDK 结构示例**：
```
smm-sdk/
├── include/              # 头文件目录
│   ├── smm.h            # 主头文件
│   └── smm_types.h       # 类型定义
├── lib/                  # 库文件目录
│   ├── smm.dll          # 动态库
│   ├── smm.lib          # 导入库
│   └── smm_static.lib   # 静态库（可选）
├── docs/                 # 文档目录
│   ├── API.md           # API 参考
│   ├── GettingStarted.md # 快速开始
│   └── Examples.md      # 示例说明
├── examples/             # 示例代码
│   ├── basic_usage.cpp
│   └── advanced_usage.cpp
└── README.md            # SDK 说明
```

### 1.3 DLL vs 静态库

| 特性         | DLL（动态库）          | 静态库（.lib）         |
| ------------ | ---------------------- | ---------------------- |
| **链接时机** | 运行时加载             | 编译时链接             |
| **文件大小** | 程序小，DLL 单独存在   | 程序大，库代码嵌入程序 |
| **更新**     | 替换 DLL 即可          | 需要重新编译程序       |
| **内存占用** | 多个程序共享同一份 DLL | 每个程序独立占用       |
| **部署**     | 需要同时部署 DLL       | 只需部署程序           |

**推荐**：对于你的项目，建议使用 **DLL**，因为：
- 内存池管理是通用功能，多个程序可能都需要
- 更新算法时，只需替换 DLL，不需要重新编译所有程序

## 二、为什么要把你的项目做成 DLL/SDK

### 2.1 当前项目结构

```
Shared-Memory-Manager/
├── server/                    # 服务器程序（REPL + TCP）
│   ├── main.cpp              # 主程序
│   ├── command/              # 命令处理（REPL）
│   ├── shared_memory_pool/   # 核心内存池 ⭐
│   ├── persistence/          # 持久化 ⭐
│   └── network/              # TCP 服务器
└── client/                    # 客户端示例
```

### 2.2 改造后的结构（DLL/SDK）

```
Shared-Memory-Manager/
├── core/                     # 核心库（编译成 DLL）
│   ├── shared_memory_pool/   # 内存池核心
│   ├── persistence/          # 持久化
│   └── api/                  # 对外 API（C 接口）
│       ├── smm_api.h         # API 头文件
│       └── smm_api.cpp       # API 实现
├── server/                   # 服务器程序（使用 DLL）
│   ├── main.cpp              # 主程序
│   ├── command/              # REPL 命令处理
│   └── network/              # TCP 服务器
├── sdk/                      # SDK 发布包
│   ├── include/              # 头文件
│   ├── lib/                  # 库文件
│   ├── docs/                 # 文档
│   └── examples/             # 示例代码
└── client/                   # 客户端（使用 SDK）
```

### 2.3 改造的好处

1. **代码复用**
   - 核心功能（内存池、持久化）可以被多个程序使用
   - 不需要复制代码

2. **模块化**
   - 核心逻辑和业务逻辑分离
   - 更容易维护和测试

3. **多语言支持**
   - C/C++ 程序可以直接使用
   - C#、Python、Java 等可以通过 FFI（Foreign Function Interface）调用

4. **版本管理**
   - 可以独立更新核心库
   - 保持 API 兼容性

5. **商业化**
   - 可以只提供 SDK，不提供源码
   - 保护核心算法

## 三、API 设计原则

### 3.1 C API vs C++ API

#### C API（推荐，跨语言兼容）

**优点**：
- ✅ 语言无关，任何语言都能调用
- ✅ ABI 稳定，不同编译器兼容
- ✅ 接口简单，易于理解

**缺点**：
- ❌ 需要手动管理资源
- ❌ 没有面向对象的便利性

**示例**：
```c
// C API 示例
typedef void* SMM_PoolHandle;  // 句柄类型

// 创建内存池
SMM_PoolHandle smm_create_pool(size_t pool_size);

// 分配内存
int smm_alloc(SMM_PoolHandle pool, const char* description, 
              const void* data, size_t data_size, char* memory_id_out);

// 释放内存
int smm_free(SMM_PoolHandle pool, const char* memory_id);

// 读取内存
int smm_read(SMM_PoolHandle pool, const char* memory_id, 
             void* buffer, size_t buffer_size, size_t* actual_size);

// 销毁内存池
void smm_destroy_pool(SMM_PoolHandle pool);
```

#### C++ API（可选，面向 C++ 用户）

**优点**：
- ✅ 面向对象，使用方便
- ✅ RAII，自动资源管理
- ✅ 类型安全

**缺点**：
- ❌ 不同编译器可能不兼容（MSVC vs MinGW）
- ❌ 其他语言调用困难

**示例**：
```cpp
// C++ API 示例
class SharedMemoryPool {
public:
    static std::unique_ptr<SharedMemoryPool> Create(size_t pool_size);
    int Allocate(const std::string& description, const void* data, 
                 size_t data_size, std::string& memory_id_out);
    int Free(const std::string& memory_id);
    int Read(const std::string& memory_id, std::vector<uint8_t>& data);
    ~SharedMemoryPool();
};
```

**建议**：**优先实现 C API**，然后可以基于 C API 封装 C++ API。

### 3.2 错误处理

**错误码设计**：
```c
// 错误码定义
typedef enum {
    SMM_SUCCESS = 0,
    SMM_ERROR_INVALID_HANDLE = -1,
    SMM_ERROR_INVALID_PARAM = -2,
    SMM_ERROR_OUT_OF_MEMORY = -3,
    SMM_ERROR_NOT_FOUND = -4,
    SMM_ERROR_ALREADY_EXISTS = -5,
    SMM_ERROR_IO_FAILED = -6,
} SMM_ErrorCode;
```

**API 返回值**：
```c
// 方式1：返回错误码，数据通过参数返回
SMM_ErrorCode smm_alloc(SMM_PoolHandle pool, ..., char* memory_id_out);

// 方式2：返回指针，NULL 表示失败，通过 smm_get_last_error() 获取错误码
char* smm_alloc(SMM_PoolHandle pool, ...);
SMM_ErrorCode smm_get_last_error(void);
```

**推荐**：使用方式1，更明确，调用者必须检查错误码。

### 3.3 内存管理

**关键问题**：DLL 分配的内存，谁负责释放？

**规则**：
- **DLL 分配的内存，由 DLL 提供的函数释放**
- **调用者分配的内存，由调用者释放**

**示例**：
```c
// 读取内存：调用者提供缓冲区
int smm_read(SMM_PoolHandle pool, const char* memory_id,
             void* buffer, size_t buffer_size, size_t* actual_size);

// 或者：DLL 分配内存，调用者负责释放
void* smm_read_alloc(SMM_PoolHandle pool, const char* memory_id, 
                     size_t* actual_size);
void smm_free_buffer(void* buffer);  // 释放 DLL 分配的内存
```

**推荐**：提供两种方式，让调用者选择。

## 四、具体实现步骤

### 4.1 第一步：设计 C API

**核心 API 列表**：

```c
// smm_api.h

#ifdef __cplusplus
extern "C" {
#endif

// 类型定义
typedef void* SMM_PoolHandle;
typedef enum { ... } SMM_ErrorCode;

// 生命周期管理
SMM_PoolHandle smm_create_pool(size_t pool_size);
SMM_ErrorCode smm_destroy_pool(SMM_PoolHandle pool);
SMM_ErrorCode smm_reset_pool(SMM_PoolHandle pool);

// 内存操作
SMM_ErrorCode smm_alloc(SMM_PoolHandle pool, const char* description,
                        const void* data, size_t data_size,
                        char* memory_id_out, size_t memory_id_size);
SMM_ErrorCode smm_free(SMM_PoolHandle pool, const char* memory_id);
SMM_ErrorCode smm_update(SMM_PoolHandle pool, const char* memory_id,
                         const void* new_data, size_t new_data_size);
SMM_ErrorCode smm_read(SMM_PoolHandle pool, const char* memory_id,
                       void* buffer, size_t buffer_size, size_t* actual_size);

// 查询操作
SMM_ErrorCode smm_get_status(SMM_PoolHandle pool, 
                              SMM_StatusInfo* status_out);
SMM_ErrorCode smm_get_memory_info(SMM_PoolHandle pool, const char* memory_id,
                                  SMM_MemoryInfo* info_out);

// 紧凑操作
SMM_ErrorCode smm_compact(SMM_PoolHandle pool);

// 持久化
SMM_ErrorCode smm_save(SMM_PoolHandle pool, const char* filename);
SMM_ErrorCode smm_load(SMM_PoolHandle pool, const char* filename);

// 错误处理
SMM_ErrorCode smm_get_last_error(void);
const char* smm_get_error_string(SMM_ErrorCode error);

#ifdef __cplusplus
}
#endif
```

### 4.2 第二步：实现 C API 包装层

**包装层作用**：将 C++ 的 `SharedMemoryPool` 类包装成 C API

```cpp
// smm_api.cpp

#include "smm_api.h"
#include "../shared_memory_pool/shared_memory_pool.h"
#include "../persistence/persistence.h"
#include <string>
#include <map>

// 全局错误码
static thread_local SMM_ErrorCode g_last_error = SMM_SUCCESS;

// 句柄到对象的映射
static std::map<SMM_PoolHandle, SharedMemoryPool*> g_pools;

SMM_PoolHandle smm_create_pool(size_t pool_size) {
    try {
        // 注意：这里需要修改 SharedMemoryPool 支持自定义大小
        // 或者固定使用 1GB
        auto* pool = new SharedMemoryPool();
        if (!pool->Init()) {
            delete pool;
            g_last_error = SMM_ERROR_OUT_OF_MEMORY;
            return nullptr;
        }
        SMM_PoolHandle handle = static_cast<SMM_PoolHandle>(pool);
        g_pools[handle] = pool;
        g_last_error = SMM_SUCCESS;
        return handle;
    } catch (...) {
        g_last_error = SMM_ERROR_OUT_OF_MEMORY;
        return nullptr;
    }
}

SMM_ErrorCode smm_alloc(SMM_PoolHandle pool, const char* description,
                        const void* data, size_t data_size,
                        char* memory_id_out, size_t memory_id_size) {
    if (!pool || !description || !data || !memory_id_out) {
        g_last_error = SMM_ERROR_INVALID_PARAM;
        return g_last_error;
    }
    
    auto it = g_pools.find(pool);
    if (it == g_pools.end()) {
        g_last_error = SMM_ERROR_INVALID_HANDLE;
        return g_last_error;
    }
    
    SharedMemoryPool* smp = it->second;
    
    // 生成 memory_id
    std::string memory_id = smp->GenerateNextMemoryId();
    
    // 分配内存
    int result = smp->AllocateBlock(memory_id, description, data, data_size);
    if (result < 0) {
        g_last_error = SMM_ERROR_OUT_OF_MEMORY;
        return g_last_error;
    }
    
    // 复制 memory_id 到输出缓冲区
    if (memory_id.size() >= memory_id_size) {
        g_last_error = SMM_ERROR_INVALID_PARAM;
        return g_last_error;
    }
    strncpy(memory_id_out, memory_id.c_str(), memory_id_size - 1);
    memory_id_out[memory_id_size - 1] = '\0';
    
    g_last_error = SMM_SUCCESS;
    return SMM_SUCCESS;
}

SMM_ErrorCode smm_free(SMM_PoolHandle pool, const char* memory_id) {
    if (!pool || !memory_id) {
        g_last_error = SMM_ERROR_INVALID_PARAM;
        return g_last_error;
    }
    
    auto it = g_pools.find(pool);
    if (it == g_pools.end()) {
        g_last_error = SMM_ERROR_INVALID_HANDLE;
        return g_last_error;
    }
    
    SharedMemoryPool* smp = it->second;
    if (!smp->FreeByMemoryId(memory_id)) {
        g_last_error = SMM_ERROR_NOT_FOUND;
        return g_last_error;
    }
    
    g_last_error = SMM_SUCCESS;
    return SMM_SUCCESS;
}

SMM_ErrorCode smm_destroy_pool(SMM_PoolHandle pool) {
    if (!pool) {
        g_last_error = SMM_ERROR_INVALID_PARAM;
        return g_last_error;
    }
    
    auto it = g_pools.find(pool);
    if (it == g_pools.end()) {
        g_last_error = SMM_ERROR_INVALID_HANDLE;
        return g_last_error;
    }
    
    delete it->second;
    g_pools.erase(it);
    
    g_last_error = SMM_SUCCESS;
    return SMM_SUCCESS;
}

// ... 其他 API 实现类似
```

### 4.3 第三步：导出 DLL 函数

**Windows 导出方式**：

```cpp
// smm_api.h
#ifdef SMM_BUILDING_DLL
    #define SMM_API __declspec(dllexport)
#else
    #define SMM_API __declspec(dllimport)
#endif

SMM_API SMM_PoolHandle smm_create_pool(size_t pool_size);
SMM_API SMM_ErrorCode smm_destroy_pool(SMM_PoolHandle pool);
// ...
```

**编译 DLL**：
```bash
# 编译时定义 SMM_BUILDING_DLL
g++ -DSMM_BUILDING_DLL -shared -o smm.dll smm_api.cpp ... -Wl,--out-implib,smm.lib
```

### 4.4 第四步：创建 SDK 结构

```
sdk/
├── include/
│   └── smm.h              # 主头文件
├── lib/
│   ├── smm.dll           # 动态库
│   └── smm.lib           # 导入库
├── docs/
│   ├── API.md            # API 参考文档
│   ├── GettingStarted.md # 快速开始指南
│   └── Examples.md       # 示例说明
├── examples/
│   ├── basic_usage.cpp   # 基础使用示例
│   └── advanced_usage.cpp # 高级使用示例
└── README.md             # SDK 说明
```

### 4.5 第五步：编写文档和示例

**快速开始示例**：

```cpp
// examples/basic_usage.cpp
#include "smm.h"
#include <iostream>

int main() {
    // 1. 创建内存池
    SMM_PoolHandle pool = smm_create_pool(1024 * 1024 * 1024); // 1GB
    if (!pool) {
        std::cerr << "Failed to create pool: " 
                  << smm_get_error_string(smm_get_last_error()) << std::endl;
        return 1;
    }
    
    // 2. 分配内存
    char memory_id[32];
    const char* data = "Hello, World!";
    SMM_ErrorCode err = smm_alloc(pool, "Test Data", data, strlen(data),
                                   memory_id, sizeof(memory_id));
    if (err != SMM_SUCCESS) {
        std::cerr << "Allocation failed: " 
                  << smm_get_error_string(err) << std::endl;
        smm_destroy_pool(pool);
        return 1;
    }
    
    std::cout << "Allocated memory ID: " << memory_id << std::endl;
    
    // 3. 读取内存
    char buffer[1024];
    size_t actual_size;
    err = smm_read(pool, memory_id, buffer, sizeof(buffer), &actual_size);
    if (err == SMM_SUCCESS) {
        std::cout << "Read data: " << std::string(buffer, actual_size) << std::endl;
    }
    
    // 4. 释放内存
    smm_free(pool, memory_id);
    
    // 5. 销毁内存池
    smm_destroy_pool(pool);
    
    return 0;
}
```

## 五、关键注意事项

### 5.1 ABI 兼容性

**问题**：C++ 的 ABI 在不同编译器间可能不兼容

**解决方案**：
- ✅ 使用 **C API**（C ABI 稳定）
- ✅ 避免在 API 中使用 C++ 类型（如 `std::string`、`std::vector`）
- ✅ 使用 C 基本类型和指针

### 5.2 线程安全

**当前状态**：你的代码使用了 `std::mutex`，但需要明确 API 的线程安全保证

**建议**：
- 在文档中明确说明：API 是否线程安全
- 如果线程安全，说明可以并发调用
- 如果非线程安全，说明需要外部同步

### 5.3 版本管理

**API 版本**：
```c
// smm_api.h
#define SMM_VERSION_MAJOR 1
#define SMM_VERSION_MINOR 0
#define SMM_VERSION_PATCH 0

SMM_ErrorCode smm_get_version(int* major, int* minor, int* patch);
```

**兼容性策略**：
- **主版本号**：不兼容的 API 变更
- **次版本号**：新增功能，向后兼容
- **修订号**：Bug 修复，向后兼容

### 5.4 内存对齐

**问题**：不同平台可能有不同的内存对齐要求

**建议**：
- 使用标准对齐（如 `alignas(8)`）
- 在文档中说明对齐要求

### 5.5 异常处理

**问题**：C++ 异常不能跨越 DLL 边界

**解决方案**：
- ✅ 在 DLL 内部捕获所有异常
- ✅ 将异常转换为错误码返回
- ✅ 不在 API 中抛出异常

```cpp
SMM_ErrorCode smm_alloc(...) {
    try {
        // 实现
        return SMM_SUCCESS;
    } catch (const std::bad_alloc&) {
        g_last_error = SMM_ERROR_OUT_OF_MEMORY;
        return g_last_error;
    } catch (...) {
        g_last_error = SMM_ERROR_UNKNOWN;
        return g_last_error;
    }
}
```

## 六、多语言支持

### 6.1 C# 调用示例

```csharp
using System;
using System.Runtime.InteropServices;

class Program {
    [DllImport("smm.dll", CallingConvention = CallingConvention.Cdecl)]
    static extern IntPtr smm_create_pool(ulong pool_size);
    
    [DllImport("smm.dll", CallingConvention = CallingConvention.Cdecl)]
    static extern int smm_alloc(IntPtr pool, string description,
                                byte[] data, ulong data_size,
                                StringBuilder memory_id, ulong memory_id_size);
    
    static void Main() {
        IntPtr pool = smm_create_pool(1024 * 1024 * 1024);
        // ...
    }
}
```

### 6.2 Python 调用示例

```python
import ctypes

# 加载 DLL
smm = ctypes.CDLL('smm.dll')

# 定义函数签名
smm.smm_create_pool.argtypes = [ctypes.c_ulonglong]
smm.smm_create_pool.restype = ctypes.c_void_p

smm.smm_alloc.argtypes = [ctypes.c_void_p, ctypes.c_char_p,
                          ctypes.POINTER(ctypes.c_ubyte), ctypes.c_ulonglong,
                          ctypes.c_char_p, ctypes.c_ulonglong]
smm.smm_alloc.restype = ctypes.c_int

# 使用
pool = smm.smm_create_pool(1024 * 1024 * 1024)
# ...
```

### 6.3 Java 调用示例（JNI）

需要创建 JNI 包装层，将 C API 包装成 Java 类。

## 七、构建脚本

### 7.1 编译 DLL（MinGW）

```bash
# build_dll.bat
@echo off
g++ -DSMM_BUILDING_DLL -shared ^
    -o smm.dll ^
    smm_api.cpp ^
    shared_memory_pool/shared_memory_pool.cpp ^
    persistence/persistence.cpp ^
    -Wl,--out-implib,smm.lib ^
    -std=c++17
```

### 7.2 使用 DLL 的程序编译

```bash
# build_app.bat
@echo off
g++ -o app.exe ^
    main.cpp ^
    -Lsdk/lib -lsmm ^
    -Isdk/include ^
    -std=c++17
```

## 八、测试

### 8.1 单元测试

为每个 API 函数编写测试：
- 正常情况测试
- 错误情况测试（无效参数、内存不足等）
- 边界情况测试

### 8.2 集成测试

测试完整流程：
- 创建 → 分配 → 读取 → 更新 → 释放 → 销毁

### 8.3 压力测试

- 大量并发分配
- 长时间运行
- 内存泄漏检测

## 九、发布清单

### SDK 发布包应包含：

- [ ] **头文件**（`include/smm.h`）
- [ ] **库文件**（`lib/smm.dll`, `lib/smm.lib`）
- [ ] **API 文档**（`docs/API.md`）
- [ ] **快速开始指南**（`docs/GettingStarted.md`）
- [ ] **示例代码**（`examples/`）
- [ ] **README**（说明如何安装和使用）
- [ ] **版本信息**（`VERSION.txt`）
- [ ] **许可证**（`LICENSE`）

## 十、总结

### 改造优先级

1. **第一阶段**：设计 C API，实现包装层
2. **第二阶段**：编译 DLL，编写基础文档
3. **第三阶段**：完善文档，添加示例代码
4. **第四阶段**：多语言绑定，性能优化

### 关键要点

- ✅ 使用 **C API** 保证跨语言兼容
- ✅ 明确的错误处理和资源管理
- ✅ 完整的文档和示例
- ✅ 版本管理和兼容性策略
- ✅ 线程安全说明

### 参考资源

- Windows DLL 开发指南
- C ABI 规范
- JNI 开发指南（如需 Java 支持）
- Python ctypes 文档（如需 Python 支持）
