#include "shared_memory_pool.h"
#include <cstring>
#include <algorithm>
#include <fstream>

bool SharedMemoryPool::Init() {
    try {
        pool_.assign(kPoolSize, 0);
        Reset();
        return true;
    } catch (...) {
        return false;
    }
}

void SharedMemoryPool::Reset() {
    std::fill(pool_.begin(), pool_.end(), 0);
    free_block_count = kBlockCount;
    for (size_t i = 0; i < kBlockCount; i++) {
        used_map.set(i, false);
    }
    for (auto& m : meta_) {
        m = BlockMeta{};
    }
    user_block_info.clear();
}

bool SharedMemoryPool::SetMeta(size_t blockId, const std::string& user) {
    if (used_map[blockId])
        return false;
    auto& m = meta_[blockId];
    m.used = true;
    m.user = user;
    free_block_count--;
    used_map.set(blockId, true);
    return true;
}

void SharedMemoryPool::SetMetaForLoad(size_t blockId, const std::string& user) {
    auto& m = meta_[blockId];
    m.used = true;
    m.user = user;
    // 不修改 free_block_count 和 used_map，因为这些已经在加载时设置好了
}

const std::map<std::string, std::pair<size_t, size_t>>& SharedMemoryPool::GetUserBlockInfo() const {
    return user_block_info;
}

void SharedMemoryPool::UpdateUserBlockCount(const std::string& user, size_t newBlockCount) {
    auto it = user_block_info.find(user);
    if (it != user_block_info.end()) {
        size_t oldBlockCount = it->second.second;
        it->second.second = newBlockCount;
        // 更新空闲块计数
        if (newBlockCount < oldBlockCount) {
            free_block_count += (oldBlockCount - newBlockCount);
        } else if (newBlockCount > oldBlockCount) {
            free_block_count -= (newBlockCount - oldBlockCount);
        }
    }
}

const SharedMemoryPool::BlockMeta& SharedMemoryPool::GetMeta(size_t blockId) const {
    return meta_[blockId];
}

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

void SharedMemoryPool::Compact() {
    // 找到第一个空闲块的位置
    size_t freePos = 0;
    // 从前往后遍历，将已使用的块移动到前面
    for (size_t i = 0; i < kBlockCount; ++i) {
        if (used_map[i]) {
            // 如果当前块已使用，且不在正确位置，需要移动
            if (i != freePos) {
                // 移动数据
                std::memcpy(pool_.data() + freePos * kBlockSize, pool_.data() + i * kBlockSize,
                            kBlockSize);
                // 移动元数据
                meta_[freePos] = meta_[i];
                // 清理源位置的元数据
                meta_[i] = BlockMeta{};
                // 更新 used_map
                used_map.set(freePos, true);
                used_map.set(i, false);
                user_block_info[meta_[freePos].user].first =
                    std::min(user_block_info[meta_[freePos].user].first, freePos);
            }
            freePos++;
        }
    }
}

// 分配内存
int SharedMemoryPool::AllocateBlock(const std::string& user, const void* data, size_t dataSize) {
    if (dataSize == 0 || user.empty() || data == nullptr) {
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
        std::memcpy(pool_.data() + blockId * kBlockSize,
                    static_cast<const uint8_t*>(data) + bytesWritten, bytesToWrite);

        // 如果块没有写满，剩余部分清零
        if (bytesToWrite < kBlockSize) {
            std::memset(pool_.data() + blockId * kBlockSize + bytesToWrite, 0,
                        kBlockSize - bytesToWrite);
        }

        // 设置元数据
        meta_[blockId].used = true;
        meta_[blockId].user = user;
        used_map.set(blockId, true);
        free_block_count--;

        bytesWritten += bytesToWrite;
    }

    user_block_info[user].first = startBlock;
    user_block_info[user].second = requiredBlocks;

    return startBlock;
}

bool SharedMemoryPool::FreeByUser(const std::string& user) {
    if (user_block_info.find(user) == user_block_info.end())
        return false;
    const auto& blockInfo = user_block_info[user];
    size_t start = blockInfo.first;
    size_t count = blockInfo.second;
    for (size_t i = start; i < start + count; i++) {
        used_map.set(i, false);
        meta_[i] = BlockMeta{};
    }
    user_block_info.erase(user);
    free_block_count += count;
    return true;
}

bool SharedMemoryPool::FreeByBlockId(size_t blockId) {
    if (!used_map[blockId])
        return false;
    used_map.set(blockId, false);
    meta_[blockId] = BlockMeta{};
    free_block_count++;
    return true;
}

void SharedMemoryPool::ClearBlockMeta(size_t blockId) {
    used_map.set(blockId, false);
    meta_[blockId].used = false;
    meta_[blockId].user.clear();
}

std::string SharedMemoryPool::GetUserContentAsString(const std::string& user) const {
    // 查找用户的块信息
    auto it = user_block_info.find(user);
    if (it == user_block_info.end()) {
        return ""; // 用户没有分配内存
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
