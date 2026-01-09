#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <bitset>
#include <map>
#include <ctime>

class SharedMemoryPool {
  public:
    static constexpr size_t kPoolSize = 1024 * 1024;              // 1MB
    static constexpr size_t kBlockSize = 4096;                    // 4KB
    static constexpr size_t kBlockCount = kPoolSize / kBlockSize; // 256

    struct BlockMeta {
        bool used = false;     // 是否被使用
        std::string user = ""; // 用户(IP:端口号)
    };

    bool Init();  // 分配 1MB
    void Reset(); // 清空所有块

    // 元信息操作
    const BlockMeta& GetMeta(size_t blockId) const;
    bool SetMeta(size_t blockId, const std::string& user);
    // 用于加载时直接设置元数据（不检查 used_map，不修改 free_block_count）
    void SetMetaForLoad(size_t blockId, const std::string& user);
    const std::map<std::string, std::pair<size_t, size_t>>& GetUserBlockInfo() const;
    // 获取用户最后修改时间
    time_t GetUserLastModifiedTime(const std::string& user) const;
    std::string GetUserLastModifiedTimeString(const std::string& user) const;

    // 内存分配相关
    // 暂定一个用户只能申请一块内存
    int FindContinuousFreeBlock(size_t blockCount); // 查找连续的空闲块
    size_t GetMaxContinuousFreeBlocks() const;      // 获取最大连续空闲块数
    void Compact();                                 // 紧凑内存
    int AllocateBlock(const std::string& user, const void* data,
                      size_t dataSize); // 分配内存

    // 内存释放相关
    bool FreeByUser(const std::string& user); // 释放用户所有内存
    bool FreeByBlockId(size_t blockId);       // 释放内存
    void ClearBlockMeta(size_t blockId);      // 清理块的元数据（不更新 free_block_count）

    // 用户内容查询（直接从内存池读取）
    std::string
    GetUserContentAsString(const std::string& user) const; // 获取用户内容字符串（遇到0停止）

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
    void SetUserBlockInfo(const std::map<std::string, std::pair<size_t, size_t>>& info) {
        user_block_info = info;
    }
    void UpdateUserBlockCount(const std::string& user, size_t newBlockCount);
    // 获取和设置用户最后修改时间（供持久化使用）
    const std::map<std::string, time_t>& GetUserLastModifiedTimeMap() const {
        return user_last_modified_time;
    }
    void SetUserLastModifiedTimeMap(const std::map<std::string, time_t>& timeMap) {
        user_last_modified_time = timeMap;
    }

  private:
    // 1MB内存池
    std::vector<uint8_t> pool_;                 // 1MB
    std::array<BlockMeta, kBlockCount> meta_{}; // 256个块的元信息
    // 记录空闲数据块信息
    size_t free_block_count = kBlockCount; // 空闲块数量
    std::bitset<kBlockCount> used_map;     // 记录256个块是否被使用
    // 记录用户使用情况
    std::map<std::string, std::pair<size_t, size_t>> user_block_info; // 用户使用块起始位置+数量
    // 记录用户最后修改时间
    std::map<std::string, time_t> user_last_modified_time; // 用户最后修改时间戳
};
