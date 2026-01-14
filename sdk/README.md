# Shared Memory Management SDK

这是 Shared Memory Management 系统的 SDK 发布包。

## 目录结构

```
sdk/
├── include/          # 头文件
├── lib/              # 库文件（DLL 和导入库）
├── docs/             # 文档
└── examples/         # 示例代码
```

## 使用说明

### 1. 包含头文件

```cpp
#include "smm.h"
```

### 2. 链接库

在编译时添加：
- `-Lsdk/lib` (库文件路径)
- `-lsmm` (链接 smm 库)

### 3. 运行时

确保 `smm.dll` 在可执行文件的路径中，或者添加到系统 PATH。

## 快速开始

参考 `examples/` 目录中的示例代码。

## 文档

详细 API 文档请参考 `docs/API.md`。
