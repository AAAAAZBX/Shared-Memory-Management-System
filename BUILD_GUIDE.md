# 构建指南 - PowerShell 执行批处理文件

## 问题说明

在 PowerShell 中直接执行 `.bat` 文件时，可能会出现以下错误：
- `'ho' 不是内部或外部命令`（`echo` 被截断）
- `'kdir' 不是内部或外部命令`（`mkdir` 被截断）
- `此时不应有 do`（`for` 循环解析错误）

这是因为 PowerShell 会尝试用 PowerShell 语法解析批处理文件，导致命令被错误解析。

## 解决方案

### 方式一：使用 `cmd /c` 执行（推荐）

**在 PowerShell 中执行批处理文件时，必须使用 `cmd /c`：**

```powershell
# 编译 DLL
cd core
cmd /c build_dll.bat

# 编译 SDK
cd ..\sdk
cmd /c build.bat

# 运行服务器
cd ..\server
cmd /c run.bat
```

### 方式二：使用 PowerShell 脚本

**已为每个批处理文件创建了对应的 PowerShell 脚本：**

```powershell
# 编译 DLL
cd core
.\build_dll.ps1

# 编译 SDK
cd ..\sdk
.\build.ps1
```

### 方式三：直接打开 cmd.exe

**最简单的方式是直接使用 cmd.exe：**

1. 按 `Win + R`，输入 `cmd`，回车
2. 切换到项目目录：
   ```cmd
   cd D:\Q\demo\Shared-Memory-Manager
   ```
3. 执行批处理文件：
   ```cmd
   cd core
   build_dll.bat
   ```

## 快速参考

### 编译 DLL

```powershell
# PowerShell 方式
cd core
cmd /c build_dll.bat
# 或
.\build_dll.ps1
```

### 编译 SDK

```powershell
# PowerShell 方式
cd sdk
cmd /c build.bat
# 或
.\build.ps1
```

### 运行服务器

```powershell
# PowerShell 方式
cd server
cmd /c run.bat
```

## 为什么会出现这个问题？

1. **编码问题**：批处理文件使用 ANSI/GBK 编码，PowerShell 默认使用 UTF-8
2. **语法差异**：PowerShell 和 cmd.exe 的语法不同，PowerShell 会尝试解析批处理文件
3. **命令解析**：PowerShell 会将某些命令（如 `echo`、`mkdir`）解析为 PowerShell 命令，导致错误

## 最佳实践

**在 PowerShell 中执行批处理文件时，始终使用 `cmd /c`：**

```powershell
cmd /c "path\to\script.bat"
```

这样可以确保批处理文件在正确的环境中执行，避免解析错误。
