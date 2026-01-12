# 内存池大小问题解决方案

## 问题分析

当前 `memory_pool.dat` 文件大小为 100MB+，这是因为：

1. **内存池配置**：`kPoolSize = 1024 * 1024 * 100 = 100MB`
2. **持久化文件内容**：
   - 文件头（约 40 字节）
   - 元数据数组（25,600 个 BlockMeta，每个包含 bool + size_t + string）
   - used_map（bitset，约 3.2KB）
   - user_block_info（用户映射表）
   - user_last_modified_time（时间戳映射）
   - **内存池数据（100MB）** ← 主要占用

所以文件大小约 100MB+ 是**正常的**。

## 解决方案

### 方案 1：保持 100MB，不提交到 Git（推荐）✅

**优点**：
- 支持更大的内存需求
- 不需要修改代码

**已完成的步骤**：
1. ✅ 创建了 `.gitignore`，忽略 `memory_pool.dat`
2. ✅ 从 Git 索引中移除了该文件
3. ✅ 从 Git 历史中移除了该文件

**下一步**：
```bash
git push --force origin main
```

### 方案 2：减小内存池到 1MB（如果不需要大内存）

如果 100MB 太大，可以改回 1MB（原始设计）：

**修改 `server/shared_memory_pool/shared_memory_pool.h`**：
```cpp
// 从：
static constexpr size_t kPoolSize = 1024 * 1024 * 100;        // 2GB

// 改为：
static constexpr size_t kPoolSize = 1024 * 1024;             // 1MB
```

**影响**：
- 持久化文件大小：约 1MB+
- 块数量：256 块（而不是 25,600 块）
- 最大可分配内存：约 1MB

**注意**：如果已有 100MB 的持久化文件，需要先删除它：
```bash
rm server/memory_pool.dat
```

## 建议

**推荐使用方案 1**，因为：
1. `.gitignore` 已正确配置，文件不会被提交
2. 100MB 的内存池可以支持更大的数据
3. 持久化文件是运行时数据，不应该提交到版本控制

## 验证

检查 `.gitignore` 是否生效：
```bash
git status
# 应该看不到 memory_pool.dat
```

检查文件是否在 Git 历史中：
```bash
git log --all --pretty=format: --name-only --diff-filter=A | grep memory_pool.dat
# 应该没有输出（已从历史中移除）
```
