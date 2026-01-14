#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <bitset>
#include <map>
#include <ctime>

class SharedMemoryPool {
  public:
    static constexpr size_t kPoolSize = 1024 * 1024 * 1024;       // 1GB
    static constexpr size_t kBlockSize = 4096;                    // 4KB
    static constexpr size_t kBlockCount = kPoolSize / kBlockSize; // 512K块

    struct BlockMeta {
        bool used = false;            // 是否被使用
        std::string memory_id = "";   // 内存ID (memory_00001, memory_00002, ...)
        std::string description = ""; // 内容描述
    };

    bool Init();  // 分配 1MB
    void Reset(); // 清空所有块

    // 元信息操作
    const BlockMeta& GetMeta(size_t blockId) const;
    bool SetMeta(size_t blockId, const std::string& memory_id, const std::string& description);
    // 用于加载时直接设置元数据（不检查 used_map，不修改 free_block_count）
    void SetMetaForLoad(size_t blockId, const std::string& memory_id,
                        const std::string& description);
    const std::map<std::string, std::pair<size_t, size_t>>& GetMemoryInfo() const;
    // 获取内存最后修改时间
    time_t GetMemoryLastModifiedTime(const std::string& memory_id) const;
    std::string GetMemoryLastModifiedTimeString(const std::string& memory_id) const;
    // 生成下一个可用的 memory_id（O(1) 时间复杂度）
    std::string GenerateNextMemoryId() const;
    // 初始化 Memory ID 计数器（从已存在的 memory_info 中找出最大值）
    void InitializeMemoryIdCounter();

    // 内存分配相关
    int FindContinuousFreeBlock(size_t blockCount); // 查找连续的空闲块
    size_t GetMaxContinuousFreeBlocks() const;      // 获取最大连续空闲块数
    void Compact();                                 // 紧凑内存
    int AllocateBlock(const std::string& memory_id, const std::string& description,
                      const void* data, size_t dataSize); // 分配内存

    // 内存释放相关
    bool FreeByMemoryId(const std::string& memory_id); // 释放指定内存ID的所有内存
    bool FreeByBlockId(size_t blockId);                // 释放内存
    void ClearBlockMeta(size_t blockId);               // 清理块的元数据（不更新 free_block_count）

    // 内存内容查询（直接从内存池读取）
    std::string
    GetMemoryContentAsString(const std::string& memory_id) const; // 获取内存内容字符串（遇到0停止）

    // 持久化相关
    bool SaveToFile(const std::string& filename) const;                // 保存到文件
    bool LoadFromFile(const std::string& filename);                    // 从文件加载
    static constexpr const char* kDefaultSaveFile = "memory_pool.dat"; // 默认保存文件名

    // 持久化辅助接口（供 persistence.cpp 使用）
    const uint8_t* GetPoolData() const {
        return pool_.data();
    }
    uint8_t* GetPoolData() {
        return pool_.data();
    }
    size_t GetFreeBlockCount() const {
        return free_block_count;
    }
    const std::bitset<kBlockCount>& GetUsedMap() const {
        return used_map;
    }
    std::bitset<kBlockCount>& GetUsedMap() {
        return used_map;
    }
    void SetFreeBlockCount(size_t count) {
        free_block_count = count;
    }
    void SetMemoryInfo(const std::map<std::string, std::pair<size_t, size_t>>& info) {
        memory_info = info;
    }
    void UpdateMemoryBlockCount(const std::string& memory_id, size_t newBlockCount);
    // 获取和设置内存最后修改时间（供持久化使用）
    const std::map<std::string, time_t>& GetMemoryLastModifiedTimeMap() const {
        return memory_last_modified_time;
    }
    void SetMemoryLastModifiedTimeMap(const std::map<std::string, time_t>& timeMap) {
        memory_last_modified_time = timeMap;
    }
    // 更新指定内存ID的最后修改时间
    void UpdateMemoryLastModifiedTime(const std::string& memory_id) {
        memory_last_modified_time[memory_id] = std::time(nullptr);
    }

  private:
    // 内存池
    std::vector<uint8_t> pool_;   // 内存池数据
    std::vector<BlockMeta> meta_; // 块的元信息（动态分配，支持大内存池）
    // 记录空闲数据块信息
    size_t free_block_count = kBlockCount; // 空闲块数量
    std::bitset<kBlockCount> used_map;     // 记录块是否被使用
    // 记录内存使用情况
    std::map<std::string, std::pair<size_t, size_t>> memory_info; // 内存ID -> (起始块位置, 块数量)
    // 记录内存最后修改时间
    std::map<std::string, time_t> memory_last_modified_time; // 内存ID -> 最后修改时间戳
    // Memory ID 计数器（O(1) 生成 ID）
    mutable uint64_t next_memory_id_counter_ =
        1; // 下一个可用的 Memory ID 编号（使用 uint64_t 支持更大范围）

    // Base62 编码辅助函数（用于生成更紧凑的 ID）
    static std::string EncodeBase62(uint64_t num, size_t minLength = 5);
    static uint64_t DecodeBase62(const std::string& str);
};
