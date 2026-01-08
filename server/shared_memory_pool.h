#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <bitset>
#include <map>

class SharedMemoryPool {
  public:
    static constexpr size_t kPoolSize = 1024 * 1024;              // 1MB
    static constexpr size_t kBlockSize = 4096;                    // 4KB
    static constexpr size_t kBlockCount = kPoolSize / kBlockSize; // 256

    struct BlockMeta {
        bool used = false;        // 是否被使用
        std::string user = "";    // 用户(IP:端口号)
        std::string content = ""; // 内容
    };

    bool Init();  // 分配 1MB
    void Reset(); // 清空所有块

    // 元信息操作
    const BlockMeta& GetMeta(size_t blockId) const;
    bool SetMeta(size_t blockId, const std::string& user, const std::string& content);
    const std::map<std::string, std::pair<size_t, size_t>>& GetUserBlockInfo() const;

    // 内存分配相关
    // 暂定一个用户只能申请一块内存
    int FindContinuousFreeBlock(size_t blockCount); // 查找连续的空闲块
    void Compact();                                 // 紧凑内存
    int AllocateBlock(const std::string& user, const std::string& content, const void* data,
                      size_t dataSize); // 分配内存

    // 内存释放相关
    bool FreeByUser(const std::string& user); // 释放用户所有内存
    bool FreeByBlockId(size_t blockId);       // 释放内存

    // 用户内容查询（直接从内存池读取）
    std::string
    GetUserContentAsString(const std::string& user) const; // 获取用户内容字符串（遇到0停止）

  private:
    // 1MB内存池
    std::vector<uint8_t> pool_;                 // 1MB
    std::array<BlockMeta, kBlockCount> meta_{}; // 256个块的元信息
    // 记录空闲数据块信息
    size_t free_block_count = kBlockCount; // 空闲块数量
    std::bitset<kBlockCount> used_map;     // 记录256个块是否被使用
    // 记录用户使用情况
    std::map<std::string, std::pair<size_t, size_t>> user_block_info; // 用户使用块起始位置+数量
};
