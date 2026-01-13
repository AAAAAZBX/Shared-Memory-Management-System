# 内存池大小配置指南

## 问题诊断

当设置内存池为 1GB 时出现错误的原因：

### 原因分析

1. **栈溢出问题**：
   - 原代码使用 `std::array<BlockMeta, kBlockCount> meta_{}` 在栈上分配
   - 1GB 内存池 = 262,144 个块（每个块 4KB）
   - 每个 `BlockMeta` 包含两个 `std::string`，大约 50-100 字节
   - 总大小：262,144 × 50-100 字节 ≈ **13-26 MB**
   - Windows 默认栈大小通常只有 **1-2 MB**，导致栈溢出

2. **解决方案**：
   - ✅ 已将 `std::array` 改为 `std::vector<BlockMeta>`
   - ✅ 在 `Init()` 方法中动态分配（堆内存）
   - ✅ 堆内存限制由系统可用虚拟内存决定，通常远大于栈大小

## 如何设置内存池大小

### 1. 修改配置

编辑 `server/shared_memory_pool/shared_memory_pool.h`：

```cpp
// 当前配置（100MB）
static constexpr size_t kPoolSize = 1024 * 1024 * 100;  // 100MB

// 设置为 1GB
static constexpr size_t kPoolSize = 1024 * 1024 * 1024;  // 1GB

// 设置为 512MB
static constexpr size_t kPoolSize = 1024 * 1024 * 512;  // 512MB
```

### 2. 检查系统内存限制

#### Windows PowerShell 命令：

```powershell
# 查看系统总内存
Get-CimInstance Win32_ComputerSystem | Select-Object TotalPhysicalMemory

# 查看可用内存
Get-CimInstance Win32_OperatingSystem | Select-Object FreePhysicalMemory

# 查看虚拟内存
Get-CimInstance Win32_OperatingSystem | Select-Object TotalVirtualMemorySize, FreeVirtualMemory
```

#### 程序内检查：

运行诊断工具（如果已编译）：
```bash
cd server
g++ -std=c++17 check_memory_limits.cpp -o check_memory_limits.exe
.\check_memory_limits.exe
```

### 3. 内存限制说明

#### 栈内存限制：
- **默认大小**：1-2 MB（32位系统）或 2-4 MB（64位系统）
- **可调整**：通过链接器选项 `/STACK:size` 调整，但不推荐设置过大
- **问题**：`std::array` 在栈上分配，受此限制

#### 堆内存限制：
- **32位系统**：通常限制在 2-4 GB（取决于系统配置）
- **64位系统**：理论上可达 TB 级别，实际受可用物理内存和虚拟内存限制
- **优势**：`std::vector` 在堆上分配，不受栈大小限制

### 4. 推荐配置

| 内存池大小 | 块数量  | meta_ 数组大小 | 推荐系统             |
| ---------- | ------- | -------------- | -------------------- |
| 100 MB     | 25,600  | ~1.3 MB        | ✅ 所有系统           |
| 512 MB     | 131,072 | ~6.5 MB        | ✅ 64位系统           |
| 1 GB       | 262,144 | ~13 MB         | ✅ 64位系统，≥4GB RAM |
| 2 GB       | 524,288 | ~26 MB         | ⚠️ 64位系统，≥8GB RAM |

### 5. 验证配置

修改后重新编译并运行：

```bash
cd server
.\run.bat
```

如果初始化失败，检查：
1. 系统可用内存是否足够
2. 是否有其他程序占用大量内存
3. 编译输出是否有错误信息

## 技术细节

### 修复内容

**修改前**（会导致栈溢出）：
```cpp
std::array<BlockMeta, kBlockCount> meta_{};  // 栈上分配
```

**修改后**（堆上分配）：
```cpp
std::vector<BlockMeta> meta_;  // 堆上分配

// 在 Init() 中：
meta_.resize(kBlockCount);  // 动态分配
```

### 内存占用计算

对于 1GB 内存池：
- **内存池数据**：1 GB（`pool_`）
- **元信息数组**：~13-26 MB（`meta_`，取决于字符串长度）
- **位图**：~32 KB（`used_map`）
- **总计**：约 1.03 GB

## 常见问题

**Q: 为什么 100MB 可以，1GB 不行？**
A: 100MB 时 `meta_` 数组约 1.3MB，刚好在栈限制内；1GB 时约 13MB，超出栈限制。

**Q: 可以设置更大的内存池吗？**
A: 可以，只要系统有足够的虚拟内存。建议不超过系统物理内存的 50%。

**Q: 如何知道系统最多支持多大？**
A: 有以下几种方法：

#### 方法1：使用 PowerShell 脚本（推荐，最简单）

直接运行 PowerShell 脚本：
```powershell
cd server
powershell -ExecutionPolicy Bypass -File check_memory.ps1
```

或者如果 PowerShell 脚本有问题，可以直接运行以下命令：
```powershell
# 查看总物理内存（GB）
$totalRAM = (Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB
Write-Host "总物理内存: $([math]::Round($totalRAM, 2)) GB"

# 查看可用物理内存（GB）
$freeRAM = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1MB / 1024
Write-Host "可用物理内存: $([math]::Round($freeRAM, 2)) GB"

# 计算推荐值
$recommended = [math]::Round($freeRAM * 0.3, 2)
$maxSafe = [math]::Round($freeRAM * 0.5, 2)
Write-Host "推荐内存池大小: $recommended GB (可用内存的30%)"
Write-Host "最大安全值: $maxSafe GB (可用内存的50%)"
```

#### 方法1b：使用 C++ 诊断工具（可选）

如果 PowerShell 不可用，可以编译并运行 C++ 工具：
```bash
cd server
g++ -std=c++17 check_system_memory.cpp -o check_system_memory.exe
.\check_system_memory.exe
```

**注意**：如果编译失败，可能是代码中的中文字符编码问题，建议使用 PowerShell 脚本方法。

#### 方法2：使用 PowerShell 命令

```powershell
# 查看总物理内存（GB）
$totalRAM = (Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB
Write-Host "总物理内存: $([math]::Round($totalRAM, 2)) GB"

# 查看可用物理内存（GB）
$freeRAM = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1MB / 1024
Write-Host "可用物理内存: $([math]::Round($freeRAM, 2)) GB"

# 查看可用虚拟内存（GB）
$os = Get-CimInstance Win32_OperatingSystem
$freeVirtual = $os.FreeVirtualMemory / 1MB / 1024
Write-Host "可用虚拟内存: $([math]::Round($freeVirtual, 2)) GB"

# 计算推荐值
$recommended = $freeRAM * 0.3
$maxSafe = $freeRAM * 0.5
Write-Host "`n推荐内存池大小: $([math]::Round($recommended, 2)) GB (可用内存的30%)"
Write-Host "最大安全值: $([math]::Round($maxSafe, 2)) GB (可用内存的50%)"
```

#### 方法3：快速估算

- **系统内存 ≥ 8GB**：可以设置 1-2 GB 内存池
- **系统内存 4-8GB**：可以设置 512 MB - 1 GB 内存池
- **系统内存 2-4GB**：建议设置 256-512 MB 内存池
- **系统内存 < 2GB**：建议保持 100 MB 内存池

#### 方法4：实际测试

修改 `shared_memory_pool.h` 中的 `kPoolSize`，然后运行程序：
- 如果程序正常启动，说明该大小可以支持
- 如果初始化失败或崩溃，说明超出了系统限制
