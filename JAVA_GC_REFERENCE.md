# Java GC 高效回收策略参考

## 一、核心思想：不要等到没空间了再去回收

### 1.1 问题：被动回收的缺陷

**传统做法（你的当前实现）**：
```
分配内存 → 空间不足 → 触发完整紧凑 → 继续分配
```

**问题**：
- ❌ **被动触发**：只在分配失败时才回收，为时已晚
- ❌ **完整紧凑**：一次性移动所有数据，暂停时间长（1-2秒）
- ❌ **用户体验差**：分配请求被阻塞，响应延迟高
- ❌ **碎片积累**：碎片问题积累到严重程度才处理

### 1.2 Java GC 的高效回收思想

**核心原则**：**主动、增量、预测性回收**

```
监控内存状态 → 达到阈值 → 增量回收 → 继续服务
```

**关键策略**：
1. ✅ **阈值触发**：在空间使用率达到阈值时主动回收（如 70%），而不是等到 100%
2. ✅ **增量回收**：将回收工作分成小批次，每次只处理一部分
3. ✅ **后台回收**：在后台线程中持续回收，不阻塞主流程
4. ✅ **碎片预防**：定期检测和整理碎片，避免碎片积累
5. ✅ **自适应调整**：根据内存使用模式动态调整回收策略

## 二、Java GC 的高效回收机制

### 2.1 增量回收（Incremental Collection）

**原理**：将完整的 GC 工作分解为多个小步骤，每次只处理一部分内存

**优势**：
- ✅ **低延迟**：单次暂停时间从秒级降到毫秒级
- ✅ **不阻塞**：分配请求可以继续，只需等待一小部分工作完成
- ✅ **可中断**：可以在任何时候暂停，让分配请求优先

**实现思路**：
```cpp
class SharedMemoryPool {
private:
    size_t compact_cursor_ = 0;      // 当前紧凑位置
    bool compact_in_progress_ = false; // 是否正在进行增量紧凑
    size_t last_compact_time_ = 0;    // 上次紧凑时间
    
public:
    // 增量紧凑：每次只处理 maxBlocksToMove 个块
    bool CompactIncremental(size_t maxBlocksToMove = 1000) {
        if (!compact_in_progress_) {
            compact_cursor_ = 0;
            compact_in_progress_ = true;
        }
        
        size_t freePos = 0;
        size_t moved = 0;
        
        // 找到第一个空闲位置
        while (freePos < compact_cursor_ && freePos < kBlockCount && used_map[freePos]) {
            freePos++;
        }
        
        // 从上次位置继续，只移动 maxBlocksToMove 个块
        for (size_t i = std::max(compact_cursor_, freePos); 
             i < kBlockCount && moved < maxBlocksToMove; ++i) {
            if (used_map[i] && i != freePos) {
                // 移动数据
                memcpy(pool_.data() + freePos * kBlockSize,
                       pool_.data() + i * kBlockSize, kBlockSize);
                meta_[freePos] = meta_[i];
                meta_[i] = BlockMeta{};
                used_map.set(freePos, true);
                used_map.set(i, false);
                
                // 更新引用
                UpdateMemoryInfo(meta_[freePos].memory_id, freePos);
                
                freePos++;
                moved++;
            }
        }
        
        compact_cursor_ = freePos;
        
        // 检查是否完成
        if (compact_cursor_ >= kBlockCount) {
            compact_in_progress_ = false;
            free_block_count = kBlockCount - freePos;
            return true;  // 完成
        }
        
        return false;  // 未完成，需要继续
    }
};
```

**使用场景**：
```cpp
int AllocateBlock(...) {
    int startBlock = FindContinuousFreeBlock(requiredBlocks);
    
    if (startBlock == -1) {
        // 尝试增量紧凑，每次只处理 1000 个块（约 5-10ms）
        while (!CompactIncremental(1000)) {
            // 可以在这里检查是否有紧急分配请求
            // 或者让出 CPU 给其他线程
        }
        
        startBlock = FindContinuousFreeBlock(requiredBlocks);
    }
    
    return startBlock;
}
```

### 2.2 阈值触发（Threshold-Based Triggering）

**原理**：在内存使用率达到阈值时主动触发回收，而不是等到分配失败

**Java GC 的触发策略**：
- **新生代**：Eden 区使用率 > 80% 时触发 Minor GC
- **老年代**：使用率 > 70% 时开始准备 Major GC
- **堆内存**：总使用率 > 75% 时触发 Full GC

**实现思路**：
```cpp
class SharedMemoryPool {
private:
    static constexpr double kCompactThreshold = 0.75;  // 75% 使用率时触发
    static constexpr double kFragmentationThreshold = 0.5;  // 碎片率 > 50% 时触发
    
public:
    // 检查是否需要触发回收
    bool ShouldTriggerGC() const {
        // 1. 检查使用率
        double usage = 1.0 - (static_cast<double>(free_block_count) / kBlockCount);
        if (usage > kCompactThreshold) {
            return true;
        }
        
        // 2. 检查碎片率
        size_t maxFree = GetMaxContinuousFreeBlocks();
        size_t totalFree = free_block_count;
        if (totalFree > 0) {
            double fragmentation = 1.0 - (static_cast<double>(maxFree) / totalFree);
            if (fragmentation > kFragmentationThreshold) {
                return true;
            }
        }
        
        return false;
    }
    
    // 在分配时检查并触发回收
    int AllocateBlock(...) {
        // 主动检查：是否需要回收
        if (ShouldTriggerGC() && !compact_in_progress_) {
            // 启动增量紧凑
            CompactIncremental(1000);
        }
        
        // 正常分配流程
        int startBlock = FindContinuousFreeBlock(requiredBlocks);
        if (startBlock == -1) {
            // 分配失败，强制紧凑
            while (!CompactIncremental(1000)) {
                // 继续增量紧凑
            }
            startBlock = FindContinuousFreeBlock(requiredBlocks);
        }
        
        return startBlock;
    }
};
```

### 2.3 后台回收（Background Collection）

**原理**：在后台线程中持续监控和回收内存，不阻塞主流程

**Java GC 的实现**：
- **并发标记**：在应用线程运行的同时标记垃圾对象
- **并发清理**：在后台线程中清理已标记的垃圾
- **并发整理**：增量整理内存碎片

**实现思路**：
```cpp
#include <thread>
#include <atomic>

class SharedMemoryPool {
private:
    std::atomic<bool> background_compact_enabled_{false};
    std::thread background_thread_;
    std::mutex compact_mutex_;
    
public:
    void StartBackgroundCompact() {
        if (background_compact_enabled_) {
            return;
        }
        
        background_compact_enabled_ = true;
        background_thread_ = std::thread([this]() {
            BackgroundCompactLoop();
        });
    }
    
    void StopBackgroundCompact() {
        background_compact_enabled_ = false;
        if (background_thread_.joinable()) {
            background_thread_.join();
        }
    }
    
private:
    void BackgroundCompactLoop() {
        while (background_compact_enabled_) {
            // 检查是否需要回收
            if (ShouldTriggerGC()) {
                std::lock_guard<std::mutex> lock(compact_mutex_);
                
                // 增量紧凑，每次处理 500 个块（约 2-5ms）
                CompactIncremental(500);
            }
            
            // 休眠 10ms，避免过度占用 CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};
```

**优势**：
- ✅ **零延迟**：主流程完全不受影响
- ✅ **持续优化**：内存状态持续改善
- ✅ **自适应**：根据内存使用情况自动调整

### 2.4 碎片检测与预防（Fragmentation Detection & Prevention）

**原理**：定期检测内存碎片程度，在碎片严重之前就开始整理

**碎片指标**：
- **最大连续空闲块**：`GetMaxContinuousFreeBlocks()`
- **总空闲块数**：`free_block_count`
- **碎片率**：`1 - (最大连续空闲块 / 总空闲块数)`

**实现思路**：
```cpp
class SharedMemoryPool {
private:
    static constexpr double kFragmentationThreshold = 0.5;  // 碎片率阈值
    size_t last_fragmentation_check_ = 0;
    
public:
    // 检查碎片程度
    double GetFragmentationRatio() const {
        size_t maxFree = GetMaxContinuousFreeBlocks();
        size_t totalFree = free_block_count;
        
        if (totalFree == 0) {
            return 0.0;
        }
        
        return 1.0 - (static_cast<double>(maxFree) / totalFree);
    }
    
    // 是否需要整理碎片
    bool IsFragmented() const {
        return GetFragmentationRatio() > kFragmentationThreshold;
    }
    
    // 在分配时检查碎片
    int AllocateBlock(...) {
        // 定期检查碎片（每 1000 次分配检查一次）
        if (++allocation_count_ % 1000 == 0) {
            if (IsFragmented() && !compact_in_progress_) {
                // 启动增量紧凑
                CompactIncremental(1000);
            }
        }
        
        // 正常分配流程
        // ...
    }
};
```

### 2.5 自适应回收（Adaptive Collection）

**原理**：根据内存使用模式动态调整回收策略和频率

**Java GC 的自适应策略**：
- **动态阈值**：根据历史数据调整触发阈值
- **频率调整**：根据回收效果调整回收频率
- **策略切换**：根据内存使用模式选择最优策略

**实现思路**：
```cpp
class SharedMemoryPool {
private:
    struct GCStats {
        size_t gc_count = 0;
        size_t total_gc_time_ms = 0;
        size_t freed_blocks = 0;
        double avg_fragmentation = 0.0;
    };
    
    GCStats gc_stats_;
    double adaptive_threshold_ = 0.75;  // 自适应阈值
    
public:
    // 记录 GC 统计信息
    void RecordGC(size_t freed_blocks, size_t gc_time_ms) {
        gc_stats_.gc_count++;
        gc_stats_.total_gc_time_ms += gc_time_ms;
        gc_stats_.freed_blocks += freed_blocks;
        
        // 更新平均碎片率
        double current_frag = GetFragmentationRatio();
        gc_stats_.avg_fragmentation = 
            (gc_stats_.avg_fragmentation * (gc_stats_.gc_count - 1) + current_frag) 
            / gc_stats_.gc_count;
        
        // 自适应调整阈值
        AdaptThreshold();
    }
    
    // 自适应调整阈值
    void AdaptThreshold() {
        // 如果 GC 频率过高，提高阈值（减少 GC 频率）
        if (gc_stats_.gc_count > 100) {
            double avg_gc_interval = 
                static_cast<double>(gc_stats_.total_gc_time_ms) / gc_stats_.gc_count;
            
            if (avg_gc_interval < 10) {  // GC 太频繁
                adaptive_threshold_ = std::min(0.90, adaptive_threshold_ + 0.05);
            } else if (avg_gc_interval > 100) {  // GC 太少
                adaptive_threshold_ = std::max(0.60, adaptive_threshold_ - 0.05);
            }
        }
    }
    
    // 使用自适应阈值
    bool ShouldTriggerGC() const {
        double usage = 1.0 - (static_cast<double>(free_block_count) / kBlockCount);
        return usage > adaptive_threshold_;
    }
};
```

## 三、Java GC 的回收算法优化

### 3.1 标记-复制优化（Mark-Copy Optimization）

**原理**：对于存活对象少的区域，使用复制算法可以快速清理

**适用场景**：
- 存活对象 < 30% 的区域
- 新分配的区域（大部分对象很快死亡）

**实现思路**：
```cpp
// 快速清理：将存活对象复制到新位置，清空原区域
void QuickClean(size_t startBlock, size_t endBlock) {
    size_t newPos = startBlock;
    
    // 第一遍：复制存活对象
    for (size_t i = startBlock; i < endBlock; ++i) {
        if (used_map[i]) {
            if (i != newPos) {
                memcpy(pool_.data() + newPos * kBlockSize,
                       pool_.data() + i * kBlockSize, kBlockSize);
                meta_[newPos] = meta_[i];
                UpdateMemoryInfo(meta_[newPos].memory_id, newPos);
            }
            newPos++;
        }
    }
    
    // 第二遍：清空剩余区域
    for (size_t i = newPos; i < endBlock; ++i) {
        meta_[i] = BlockMeta{};
        used_map.set(i, false);
    }
    
    free_block_count += (endBlock - newPos);
}
```

### 3.2 增量标记（Incremental Marking）

**原理**：将标记阶段分成多个小步骤，每次只标记一部分对象

**优势**：
- ✅ **低延迟**：单次标记时间短
- ✅ **可中断**：可以在标记过程中处理分配请求

**实现思路**：
```cpp
class SharedMemoryPool {
private:
    size_t mark_cursor_ = 0;
    std::bitset<kBlockCount> mark_map_;  // 标记位图
    
public:
    // 增量标记：每次只标记 maxBlocksToMark 个块
    bool MarkIncremental(size_t maxBlocksToMark = 1000) {
        size_t marked = 0;
        
        for (size_t i = mark_cursor_; i < kBlockCount && marked < maxBlocksToMark; ++i) {
            if (used_map[i]) {
                // 标记为存活
                mark_map_.set(i, true);
                marked++;
            }
        }
        
        mark_cursor_ += marked;
        
        return mark_cursor_ >= kBlockCount;  // 是否完成
    }
    
    // 增量清除：只清除未标记的块
    void SweepIncremental(size_t maxBlocksToSweep = 1000) {
        size_t swept = 0;
        
        for (size_t i = 0; i < kBlockCount && swept < maxBlocksToSweep; ++i) {
            if (used_map[i] && !mark_map_[i]) {
                // 未标记，清除
                FreeByBlockId(i);
                swept++;
            }
        }
    }
};
```

### 3.3 并发回收（Concurrent Collection）

**原理**：在应用线程运行的同时进行垃圾回收

**Java GC 的实现**：
- **并发标记**：与应用线程并发标记对象
- **并发清理**：在后台线程中清理垃圾
- **写屏障**：处理并发标记时的对象修改

**实现思路**（简化版）：
```cpp
class SharedMemoryPool {
private:
    std::atomic<bool> concurrent_marking_{false};
    std::thread concurrent_thread_;
    
public:
    // 启动并发标记
    void StartConcurrentMarking() {
        if (concurrent_marking_) {
            return;
        }
        
        concurrent_marking_ = true;
        concurrent_thread_ = std::thread([this]() {
            while (concurrent_marking_) {
                // 增量标记
                if (MarkIncremental(500)) {
                    // 标记完成，开始清理
                    SweepIncremental(1000);
                    concurrent_marking_ = false;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }
};
```

## 四、你的项目改进方案

### 4.1 方案 1：阈值触发 + 增量紧凑（推荐，易于实现）

**核心思想**：
- 在内存使用率达到 75% 时主动触发回收
- 使用增量紧凑，每次只处理 1000 个块（约 5-10ms）

**实现步骤**：
1. 添加 `compact_cursor_` 和 `compact_in_progress_` 成员变量
2. 实现 `CompactIncremental()` 函数
3. 在 `AllocateBlock()` 中添加阈值检查
4. 修改 `Compact()` 调用为增量紧凑

**优势**：
- ✅ 实现简单，改动小
- ✅ 立即见效，延迟降低 100-200 倍
- ✅ 不阻塞分配请求

### 4.2 方案 2：后台回收（推荐，效果显著）

**核心思想**：
- 在后台线程中持续监控内存状态
- 达到阈值时自动启动增量紧凑
- 主流程完全不受影响

**实现步骤**：
1. 添加后台线程管理
2. 实现 `BackgroundCompactLoop()` 函数
3. 在 `Init()` 时启动后台线程
4. 在 `Reset()` 时停止后台线程

**优势**：
- ✅ 零延迟，用户体验最佳
- ✅ 内存状态持续优化
- ✅ 完全自动化

### 4.3 方案 3：碎片检测 + 预防性整理（简单有效）

**核心思想**：
- 定期检测碎片率
- 碎片率 > 50% 时启动增量紧凑
- 避免碎片积累到严重程度

**实现步骤**：
1. 实现 `GetFragmentationRatio()` 函数
2. 在分配时定期检查碎片率
3. 达到阈值时启动增量紧凑

**优势**：
- ✅ 实现简单
- ✅ 避免不必要的紧凑
- ✅ 保持内存健康状态

### 4.4 方案 4：自适应回收（高级优化）

**核心思想**：
- 记录 GC 统计信息
- 根据历史数据动态调整阈值和频率
- 自动优化回收策略

**实现步骤**：
1. 添加 GC 统计结构
2. 实现 `RecordGC()` 和 `AdaptThreshold()` 函数
3. 在每次 GC 后更新统计信息

**优势**：
- ✅ 自动优化，无需手动调优
- ✅ 适应不同的使用模式
- ✅ 长期性能最佳

## 五、性能对比

### 当前实现（被动回收）
- **触发时机**：分配失败时（100% 使用率）
- **回收方式**：完整紧凑，一次性处理所有块
- **单次延迟**：1-2 秒（262,144 块）
- **用户体验**：分配请求被阻塞，响应延迟高

### 改进后（阈值触发 + 增量紧凑）
- **触发时机**：使用率达到 75% 时主动触发
- **回收方式**：增量紧凑，每次处理 1000 个块
- **单次延迟**：5-10 毫秒（1000 块）
- **用户体验**：延迟降低 100-200 倍，几乎无感知

### 改进后（后台回收）
- **触发时机**：后台持续监控，达到阈值自动触发
- **回收方式**：后台增量紧凑，不阻塞主流程
- **单次延迟**：0 毫秒（对用户完全透明）
- **用户体验**：零延迟，最佳体验

## 六、总结

### Java GC 高效回收的核心思想

1. **主动回收**：在达到阈值时主动触发，而不是等到分配失败
2. **增量回收**：将回收工作分成小批次，降低单次延迟
3. **后台回收**：在后台线程中持续回收，不阻塞主流程
4. **碎片预防**：定期检测和整理碎片，避免碎片积累
5. **自适应调整**：根据使用模式动态调整策略

### 优先级建议

1. **高优先级**：阈值触发 + 增量紧凑（方案 1）
   - 实现简单，立即见效
   - 延迟降低 100-200 倍

2. **中优先级**：后台回收（方案 2）
   - 效果显著，用户体验最佳
   - 需要线程管理

3. **低优先级**：碎片检测（方案 3）
   - 简单有效，避免不必要的回收
   - 可以作为补充优化

4. **可选**：自适应回收（方案 4）
   - 高级优化，长期性能最佳
   - 需要较复杂的实现

### 关键要点

- ❌ **不要**等到分配失败才回收
- ✅ **要**在达到阈值时主动回收
- ❌ **不要**一次性处理所有内存
- ✅ **要**使用增量方式分批处理
- ❌ **不要**阻塞分配请求
- ✅ **要**在后台持续优化内存

### 参考资源

- 《深入理解 Java 虚拟机》- 周志明
- Java HotSpot VM 源码
- G1 GC 论文
- ZGC 并发回收算法
