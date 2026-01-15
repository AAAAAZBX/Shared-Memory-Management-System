#include "shared_memory_pool.h"
#include <cstring>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <cctype>
#include <cstdlib>
#include <new>
#include <set>
#include <vector>

// 初始化
bool SharedMemoryPool::Init() {
    // 如果已经初始化过，先释放旧内存
    if (pool_) {
        std::free(pool_);
        pool_ = nullptr;
    }
    if (meta_) {
        // 调用每个 BlockMeta 的析构函数
        for (size_t i = 0; i < kBlockCount; ++i) {
            meta_[i].~BlockMeta();
        }
        std::free(meta_);
        meta_ = nullptr;
    }

    // 使用 malloc 分配内存池数据
    pool_ = static_cast<uint8_t*>(std::malloc(kPoolSize));
    if (!pool_) {
        return false; // 内存分配失败
    }

    // 使用 malloc 分配元信息数组
    meta_ = static_cast<BlockMeta*>(std::malloc(sizeof(BlockMeta) * kBlockCount));
    if (!meta_) {
        std::free(pool_);
        pool_ = nullptr;
        return false; // 内存分配失败
    }

    // 使用 placement new 初始化每个 BlockMeta 对象（调用构造函数）
    for (size_t i = 0; i < kBlockCount; ++i) {
        new (meta_ + i) BlockMeta();
    }

    // 初始化内存
    Reset();
    return true;
}

// 重置
void SharedMemoryPool::Reset() {
    if (pool_) {
        std::memset(pool_, 0, kPoolSize);
    }
    free_block_count = kBlockCount;
    for (size_t i = 0; i < kBlockCount; i++) {
        used_map.set(i, false);
    }
    if (meta_) {
        for (size_t i = 0; i < kBlockCount; i++) {
            meta_[i] = BlockMeta{};
        }
    }
    memory_info.clear();
    memory_last_modified_time.clear(); // Clear last modified times
    next_memory_id_counter_ = 1;       // 重置计数器
}

// 设置元数据
bool SharedMemoryPool::SetMeta(size_t blockId, const std::string& memory_id,
                               const std::string& description) {
    if (used_map[blockId])
        return false;
    auto& m = meta_[blockId];
    m.used = true;
    m.memory_id = memory_id;
    m.description = description;
    free_block_count--;
    used_map.set(blockId, true);
    return true;
}

// 设置元数据（加载时使用）
void SharedMemoryPool::SetMetaForLoad(size_t blockId, const std::string& memory_id,
                                      const std::string& description) {
    auto& m = meta_[blockId];
    m.used = true;
    m.memory_id = memory_id;
    m.description = description;
    // 不修改 free_block_count 和 used_map，因为这些已经在加载时设置好了
}

// 获取内存块信息
const std::map<std::string, std::pair<size_t, size_t>>& SharedMemoryPool::GetMemoryInfo() const {
    return memory_info;
}

// 更新内存块数量
void SharedMemoryPool::UpdateMemoryBlockCount(const std::string& memory_id, size_t newBlockCount) {
    auto it = memory_info.find(memory_id);
    if (it != memory_info.end()) {
        size_t oldBlockCount = it->second.second;
        it->second.second = newBlockCount;
        // 更新空闲块计数
        if (newBlockCount < oldBlockCount) {
            free_block_count += (oldBlockCount - newBlockCount);
        } else if (newBlockCount > oldBlockCount) {
            free_block_count -= (newBlockCount - oldBlockCount);
        }
        // 更新最后修改时间
        memory_last_modified_time[memory_id] = std::time(nullptr);
    }
}

// Base62 编码：将数字转换为 Base62 字符串（0-9, a-z, A-Z）
std::string SharedMemoryPool::EncodeBase62(uint64_t num, size_t minLength) {
    const char* base62_chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string result;

    if (num == 0) {
        result = "0";
    } else {
        while (num > 0) {
            result = base62_chars[num % 62] + result;
            num /= 62;
        }
    }

    // 填充到最小长度（左侧补0）
    while (result.length() < minLength) {
        result = "0" + result;
    }

    return result;
}

// Base62 解码：将 Base62 字符串转换为数字
uint64_t SharedMemoryPool::DecodeBase62(const std::string& str) {
    uint64_t result = 0;
    uint64_t base = 1;

    for (int i = static_cast<int>(str.length()) - 1; i >= 0; i--) {
        char c = str[i];
        uint64_t value = 0;

        if (c >= '0' && c <= '9') {
            value = c - '0';
        } else if (c >= 'a' && c <= 'z') {
            value = 10 + (c - 'a');
        } else if (c >= 'A' && c <= 'Z') {
            value = 36 + (c - 'A');
        } else {
            return 0; // 无效字符
        }

        result += value * base;
        base *= 62;
    }

    return result;
}

// 生成下一个可用的 memory_id（O(1) 时间复杂度）
std::string SharedMemoryPool::GenerateNextMemoryId() const {
    uint64_t currentId = next_memory_id_counter_++;

    // 使用 Base62 编码，自动扩展位数：
    // 5位：支持约9亿个ID（0 - 916,132,831）
    // 6位：支持约568亿个ID（916,132,832 - 56,800,235,583）
    // 7位：支持约3521亿个ID（56,800,235,584 - 3,521,614,606,207）
    size_t minLength = 5;
    if (currentId >= 916132832ULL) { // 62^5 = 916,132,832
        minLength = 6;
    }
    if (currentId >= 56800235584ULL) { // 62^6 = 56,800,235,584
        minLength = 7;
    }

    std::string encoded = EncodeBase62(currentId, minLength);
    return "memory_" + encoded;
}

// 初始化 Memory ID 计数器（从已存在的 memory_info 中找出最大值）
// 在从文件加载后调用，确保计数器大于所有已存在的 ID
// 仅支持 Base62 编码格式
void SharedMemoryPool::InitializeMemoryIdCounter() {
    uint64_t maxId = 0;

    for (const auto& entry : memory_info) {
        const std::string& id = entry.first;
        if (id.length() > 7 && id.substr(0, 7) == "memory_") {
            std::string suffix = id.substr(7);
            uint64_t currentId = DecodeBase62(suffix);

            if (currentId > 0 || suffix == "0") {
                maxId = std::max(maxId, currentId);
            }
        }
    }

    next_memory_id_counter_ = maxId + 1; // 设置为下一个可用的 ID
}

// 获取块元数据
const SharedMemoryPool::BlockMeta& SharedMemoryPool::GetMeta(size_t blockId) const {
    return meta_[blockId];
}

// 查找连续的空闲块
int SharedMemoryPool::FindContinuousFreeBlock(size_t blockCount) {
    if (blockCount > free_block_count)
        return -1;
    for (size_t i = 0; i < kBlockCount; i++) {
        if (used_map[i])
            continue;
        size_t j = i;
        while (j < kBlockCount && !used_map[j]) {
            ++j;
        }
        if (j - i >= blockCount) {
            return i;
        }
        i = j - 1;
    }
    return -1;
}

// 获取最大连续空闲块数
size_t SharedMemoryPool::GetMaxContinuousFreeBlocks() const {
    size_t maxContinuous = 0;
    size_t currentContinuous = 0;

    for (size_t i = 0; i < kBlockCount; ++i) {
        if (!used_map[i]) {
            currentContinuous++;
            maxContinuous = std::max(maxContinuous, currentContinuous);
        } else {
            currentContinuous = 0;
        }
    }

    return maxContinuous;
}

// 紧凑内存
void SharedMemoryPool::Compact() {
    size_t freePos = 0; // 下一个空闲位置
    // 记录已经处理过的 memory_id，避免重复处理
    std::set<std::string> processed;

    // 按 memory_id 为单位移动：遍历 memory_info，按原始起始位置排序
    std::vector<std::pair<std::string, std::pair<size_t, size_t>>> sortedEntries;
    for (const auto& entry : memory_info) {
        sortedEntries.push_back(entry);
    }
    // 按起始块位置排序
    std::sort(sortedEntries.begin(), sortedEntries.end(),
              [](const auto& a, const auto& b) { return a.second.first < b.second.first; });

    // 遍历每个 memory_id，移动其所有块
    for (const auto& entry : sortedEntries) {
        const std::string& memory_id = entry.first;
        size_t oldStartBlock = entry.second.first;
        size_t blockCount = entry.second.second;

        // 跳过已处理的 memory_id
        if (processed.find(memory_id) != processed.end()) {
            continue;
        }

        // 移动这个 memory_id 的所有块
        for (size_t j = 0; j < blockCount; ++j) {
            size_t srcBlock = oldStartBlock + j;
            size_t dstBlock = freePos + j;

            if (srcBlock != dstBlock) {
                // 移动数据
                memcpy(pool_ + dstBlock * kBlockSize, pool_ + srcBlock * kBlockSize, kBlockSize);
                // 移动元数据
                meta_[dstBlock] = meta_[srcBlock];
                // 清理源位置
                meta_[srcBlock] = BlockMeta{};
                // 更新 used_map
                used_map.set(dstBlock, true);
                used_map.set(srcBlock, false);
            }
        }

        // 更新 memory_info 中的起始位置
        auto it = memory_info.find(memory_id);
        if (it != memory_info.end()) {
            it->second.first = freePos;
        }

        processed.insert(memory_id);
        freePos += blockCount;
    }

    // 更新空闲块计数
    free_block_count = kBlockCount - freePos;
}

// 分配内存
int SharedMemoryPool::AllocateBlock(const std::string& memory_id, const std::string& description,
                                    const void* data, size_t dataSize) {
    if (dataSize == 0 || memory_id.empty() || data == nullptr) {
        return -1;
    }

    // 计算需要的块数（向上取整）
    size_t requiredBlocks = (dataSize + kBlockSize) / kBlockSize;

    // 检查总空闲空间是否足够
    size_t totalFreeSpace = static_cast<size_t>(free_block_count) * kBlockSize;
    if (dataSize > totalFreeSpace) {
        return -1; // 空间不足
    }

    // 尝试查找连续的空闲块
    int startBlock = FindContinuousFreeBlock(requiredBlocks);

    // 如果找不到连续块，但总空间足够，进行紧凑
    if (startBlock == -1) {
        Compact();
        // 紧凑后重新查找
        startBlock = FindContinuousFreeBlock(requiredBlocks);
        if (startBlock == -1) {
            return -1; // 紧凑后仍然找不到（理论上不应该发生）
        }
    }

    // 分配块并写入数据
    size_t bytesWritten = 0;
    for (size_t i = 0; i < requiredBlocks; ++i) {
        size_t blockId = static_cast<size_t>(startBlock) + i;
        size_t bytesToWrite = std::min(kBlockSize, dataSize - bytesWritten);

        // 写入数据
        memcpy(pool_ + blockId * kBlockSize, static_cast<const uint8_t*>(data) + bytesWritten,
               bytesToWrite);

        // 如果块没有写满，剩余部分清零
        if (bytesToWrite < kBlockSize) {
            memset(pool_ + blockId * kBlockSize + bytesToWrite, 0, kBlockSize - bytesToWrite);
        }

        // 设置元数据
        meta_[blockId].used = true;
        meta_[blockId].memory_id = memory_id;
        meta_[blockId].description = description;
        used_map.set(blockId, true);
        free_block_count--;

        bytesWritten += bytesToWrite;
    }

    memory_info[memory_id].first = startBlock;
    memory_info[memory_id].second = requiredBlocks;
    // 更新最后修改时间
    memory_last_modified_time[memory_id] = std::time(nullptr);

    return startBlock;
}

// 释放指定内存ID的所有内存
bool SharedMemoryPool::FreeByMemoryId(const std::string& memory_id) {
    if (memory_info.find(memory_id) == memory_info.end())
        return false;
    const auto& blockInfo = memory_info[memory_id];
    size_t start = blockInfo.first;
    size_t count = blockInfo.second;
    for (size_t i = start; i < start + count; i++) {
        used_map.set(i, false);
        meta_[i] = BlockMeta{};
    }
    memory_info.erase(memory_id);
    memory_last_modified_time.erase(memory_id); // 删除最后修改时间记录
    free_block_count += count;
    return true;
}

// 释放指定块
bool SharedMemoryPool::FreeByBlockId(size_t blockId) {
    if (!used_map[blockId])
        return false;
    used_map.set(blockId, false);
    meta_[blockId] = BlockMeta{};
    free_block_count++;
    return true;
}

// 清理块元数据
void SharedMemoryPool::ClearBlockMeta(size_t blockId) {
    used_map.set(blockId, false);
    meta_[blockId].used = false;
    meta_[blockId].memory_id.clear();
    meta_[blockId].description.clear();
}

// 获取内存最后修改时间
time_t SharedMemoryPool::GetMemoryLastModifiedTime(const std::string& memory_id) const {
    auto it = memory_last_modified_time.find(memory_id);
    if (it != memory_last_modified_time.end()) {
        return it->second;
    }
    return 0; // 返回0表示没有记录
}

// 获取内存最后修改时间字符串
std::string SharedMemoryPool::GetMemoryLastModifiedTimeString(const std::string& memory_id) const {
    time_t timeValue = GetMemoryLastModifiedTime(memory_id);
    if (timeValue == 0) {
        return "N/A";
    }

    std::tm* timeInfo = std::localtime(&timeValue);
    if (timeInfo == nullptr) {
        return "N/A";
    }

    std::ostringstream oss;
    oss << std::put_time(timeInfo, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// 获取内存内容字符串
std::string SharedMemoryPool::GetMemoryContentAsString(const std::string& memory_id) const {
    // 查找内存的块信息
    auto it = memory_info.find(memory_id);
    if (it == memory_info.end()) {
        return ""; // 内存ID不存在
    }

    size_t startBlock = it->second.first;
    size_t blockCount = it->second.second;
    size_t totalSize = blockCount * kBlockSize;

    // 直接从内存池读取，遇到0停止
    const uint8_t* data = pool_ + startBlock * kBlockSize;
    std::string result;

    for (size_t i = 0; i < totalSize; ++i) {
        if (data[i] == 0) {
            break; // 遇到0，字符串结尾
        }
        result += static_cast<char>(data[i]);
    }

    return result;
}
