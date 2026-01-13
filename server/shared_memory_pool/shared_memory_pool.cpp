#include "shared_memory_pool.h"
#include <cstring>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>

// 初始化
bool SharedMemoryPool::Init() {
    try {
        pool_.assign(kPoolSize, 0);
        Reset();
        return true;
    } catch (...) {
        return false;
    }
}

// 重置
void SharedMemoryPool::Reset() {
    std::fill(pool_.begin(), pool_.end(), 0);
    free_block_count = kBlockCount;
    for (size_t i = 0; i < kBlockCount; i++) {
        used_map.set(i, false);
    }
    for (auto& m : meta_) {
        m = BlockMeta{};
    }
    memory_info.clear();
    memory_last_modified_time.clear(); // Clear last modified times
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

// 生成下一个可用的 memory_id
std::string SharedMemoryPool::GenerateNextMemoryId() const {
    int maxId = 0;
    for (const auto& entry : memory_info) {
        const std::string& id = entry.first;
        if (id.length() > 7 && id.substr(0, 7) == "memory_") {
            try {
                int currentId = std::stoi(id.substr(7));
                maxId = std::max(maxId, currentId);
            } catch (...) {
                // 忽略解析错误
            }
        }
    }
    std::ostringstream oss;
    oss << "memory_" << std::setfill('0') << std::setw(5) << (maxId + 1);
    return oss.str();
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
    // 找到第一个空闲块的位置
    size_t freePos = 0;
    // 记录每个 memory_id 的新起始位置（避免重复更新）
    std::map<std::string, size_t> newStartPositions;

    // 从前往后遍历，将已使用的块移动到前面
    for (size_t i = 0; i < kBlockCount; ++i) {
        if (used_map[i]) {
            const std::string& memory_id = meta_[i].memory_id;

            // 如果当前块已使用，且不在正确位置，需要移动
            if (i != freePos) {
                // 移动数据
                memcpy(pool_.data() + freePos * kBlockSize, pool_.data() + i * kBlockSize,
                       kBlockSize);
                // 移动元数据
                meta_[freePos] = meta_[i];
                // 清理源位置的元数据
                meta_[i] = BlockMeta{};
                // 更新 used_map
                used_map.set(freePos, true);
                used_map.set(i, false);
            }

            // 更新内存信息中的起始块位置（只更新一次，对于每个 memory_id 的第一个块）
            if (!memory_id.empty()) {
                if (newStartPositions.find(memory_id) == newStartPositions.end()) {
                    // 这是该 memory_id 的第一个块，记录新起始位置
                    newStartPositions[memory_id] = freePos;
                    auto it = memory_info.find(memory_id);
                    if (it != memory_info.end()) {
                        it->second.first = freePos;
                    }
                }
            }

            freePos++;
        }
    }
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
        memcpy(pool_.data() + blockId * kBlockSize,
               static_cast<const uint8_t*>(data) + bytesWritten, bytesToWrite);

        // 如果块没有写满，剩余部分清零
        if (bytesToWrite < kBlockSize) {
            memset(pool_.data() + blockId * kBlockSize + bytesToWrite, 0,
                   kBlockSize - bytesToWrite);
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
    const uint8_t* data = pool_.data() + startBlock * kBlockSize;
    std::string result;

    for (size_t i = 0; i < totalSize; ++i) {
        if (data[i] == 0) {
            break; // 遇到0，字符串结尾
        }
        result += static_cast<char>(data[i]);
    }

    return result;
}
