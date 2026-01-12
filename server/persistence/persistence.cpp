#include "persistence.h"
#include <fstream>
#include <cstring>
#include <vector>

namespace Persistence {
// 文件格式版本号和魔数
static constexpr uint32_t kFileVersion = 3;        // 版本3：使用 memory_id 和 description 替代 user
static constexpr uint32_t kFileMagic = 0x4D454D50; // "MEMP"

// 文件头结构
struct FileHeader {
    uint32_t magic;           // 文件魔数
    uint32_t version;         // 文件格式版本
    size_t free_block_count;  // 空闲块数量
    size_t memory_info_count; // memory_info 的数量
    uint64_t reserved[4];     // 预留字段
};

bool Save(const SharedMemoryPool& smp, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    try {
        // 1. 写入文件头
        FileHeader header{};
        header.magic = kFileMagic;
        header.version = kFileVersion;

        // 获取内部数据（需要通过公共接口）
        const auto& memoryInfo = smp.GetMemoryInfo();
        header.free_block_count = 0; // 需要计算，暂时设为0
        header.memory_info_count = memoryInfo.size();

        // 计算空闲块数量（通过遍历元数据）
        for (size_t i = 0; i < SharedMemoryPool::kBlockCount; ++i) {
            const auto& meta = smp.GetMeta(i);
            if (!meta.used) {
                header.free_block_count++;
            }
        }

        file.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));

        // 2. 写入元数据数组
        for (size_t i = 0; i < SharedMemoryPool::kBlockCount; ++i) {
            const auto& meta = smp.GetMeta(i);

            bool used = meta.used;
            file.write(reinterpret_cast<const char*>(&used), sizeof(bool));

            // 写入 memory_id
            size_t memoryIdLen = meta.memory_id.size();
            file.write(reinterpret_cast<const char*>(&memoryIdLen), sizeof(size_t));
            if (memoryIdLen > 0) {
                file.write(meta.memory_id.c_str(), memoryIdLen);
            }

            // 写入 description
            size_t descLen = meta.description.size();
            file.write(reinterpret_cast<const char*>(&descLen), sizeof(size_t));
            if (descLen > 0) {
                file.write(meta.description.c_str(), descLen);
            }
        }

        // 3. 写入 used_map (需要访问内部数据，暂时跳过，通过元数据重建)
        // 或者我们需要在 SharedMemoryPool 中添加访问 used_map 的接口
        // 这里先写入一个占位符，实际使用时需要添加接口
        std::vector<uint8_t> bitsetBytes((SharedMemoryPool::kBlockCount + 7) / 8, 0);
        for (size_t i = 0; i < SharedMemoryPool::kBlockCount; ++i) {
            const auto& meta = smp.GetMeta(i);
            if (meta.used) {
                bitsetBytes[i / 8] |= (1 << (i % 8));
            }
        }
        file.write(reinterpret_cast<const char*>(bitsetBytes.data()), bitsetBytes.size());

        // 4. 写入 memory_info
        for (const auto& entry : memoryInfo) {
            size_t keyLen = entry.first.size();
            file.write(reinterpret_cast<const char*>(&keyLen), sizeof(size_t));
            file.write(entry.first.c_str(), keyLen);

            file.write(reinterpret_cast<const char*>(&entry.second.first), sizeof(size_t));
            file.write(reinterpret_cast<const char*>(&entry.second.second), sizeof(size_t));
        }

        // 5. 写入 memory_last_modified_time
        const auto& timeMap = smp.GetMemoryLastModifiedTimeMap();
        size_t timeMapCount = timeMap.size();
        file.write(reinterpret_cast<const char*>(&timeMapCount), sizeof(size_t));
        for (const auto& entry : timeMap) {
            size_t keyLen = entry.first.size();
            file.write(reinterpret_cast<const char*>(&keyLen), sizeof(size_t));
            file.write(entry.first.c_str(), keyLen);
            time_t timeValue = entry.second;
            file.write(reinterpret_cast<const char*>(&timeValue), sizeof(time_t));
        }

        // 6. 写入内存池数据
        const uint8_t* poolData = smp.GetPoolData();
        file.write(reinterpret_cast<const char*>(poolData), SharedMemoryPool::kPoolSize);

        return file.good();
    } catch (...) {
        return false;
    }
}

bool Load(SharedMemoryPool& smp, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    try {
        // 1. 读取文件头
        FileHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));

        if (header.magic != kFileMagic) {
            return false;
        }
        // 支持版本1、版本2和版本3
        if (header.version != 1 && header.version != 2 && header.version != 3) {
            return false;
        }

        // 2. 重置内存池
        smp.Reset();

        // 3. 读取元数据（先读取，暂存）
        struct MetaData {
            bool used;
            std::string memory_id;
            std::string description;
        };
        std::vector<MetaData> metaData(SharedMemoryPool::kBlockCount);

        for (size_t i = 0; i < SharedMemoryPool::kBlockCount; ++i) {
            file.read(reinterpret_cast<char*>(&metaData[i].used), sizeof(bool));

            if (header.version >= 3) {
                // 版本3：读取 memory_id 和 description
                size_t memoryIdLen;
                file.read(reinterpret_cast<char*>(&memoryIdLen), sizeof(size_t));
                metaData[i].memory_id.resize(memoryIdLen);
                if (memoryIdLen > 0) {
                    file.read(&metaData[i].memory_id[0], memoryIdLen);
                }

                size_t descLen;
                file.read(reinterpret_cast<char*>(&descLen), sizeof(size_t));
                metaData[i].description.resize(descLen);
                if (descLen > 0) {
                    file.read(&metaData[i].description[0], descLen);
                }
            } else {
                // 版本1和2：读取 user（向后兼容）
                size_t userLen;
                file.read(reinterpret_cast<char*>(&userLen), sizeof(size_t));
                std::string user(userLen, '\0');
                if (userLen > 0) {
                    file.read(&user[0], userLen);
                }
                // 将 user 转换为 memory_id（使用 user 作为 memory_id）
                metaData[i].memory_id = user;
                metaData[i].description = ""; // 旧版本没有 description
            }
        }

        // 4. 读取并设置 used_map
        std::vector<uint8_t> bitsetBytes((SharedMemoryPool::kBlockCount + 7) / 8, 0);
        file.read(reinterpret_cast<char*>(bitsetBytes.data()), bitsetBytes.size());
        // 从字节数组恢复 bitset
        auto& usedMap = smp.GetUsedMap();
        usedMap.reset(); // 先全部清零
        for (size_t i = 0; i < SharedMemoryPool::kBlockCount; ++i) {
            if (bitsetBytes[i / 8] & (1 << (i % 8))) {
                usedMap.set(i, true);
            }
        }

        // 5. 设置 free_block_count
        smp.SetFreeBlockCount(header.free_block_count);

        // 6. 设置元数据（在 used_map 设置之后）
        for (size_t i = 0; i < SharedMemoryPool::kBlockCount; ++i) {
            if (metaData[i].used) {
                smp.SetMetaForLoad(i, metaData[i].memory_id, metaData[i].description);
            }
        }

        // 7. 读取并设置 memory_info
        // 注意：旧版本文件中这个字段叫 user_info_count，但由于结构体内存布局相同，
        // 读取时会自动映射到 memory_info_count 字段
        size_t infoCount = header.memory_info_count;
        std::map<std::string, std::pair<size_t, size_t>> memoryInfo;
        for (size_t i = 0; i < infoCount; ++i) {
            size_t keyLen;
            file.read(reinterpret_cast<char*>(&keyLen), sizeof(size_t));
            std::string key(keyLen, '\0');
            if (keyLen > 0) {
                file.read(&key[0], keyLen);
            }

            size_t startBlock;
            size_t blockCount;
            file.read(reinterpret_cast<char*>(&startBlock), sizeof(size_t));
            file.read(reinterpret_cast<char*>(&blockCount), sizeof(size_t));

            memoryInfo[key] = std::make_pair(startBlock, blockCount);
        }
        smp.SetMemoryInfo(memoryInfo);

        // 8. 读取 memory_last_modified_time（版本2和3）
        if (header.version >= 2) {
            size_t timeMapCount;
            file.read(reinterpret_cast<char*>(&timeMapCount), sizeof(size_t));
            std::map<std::string, time_t> timeMap;
            for (size_t i = 0; i < timeMapCount; ++i) {
                size_t keyLen;
                file.read(reinterpret_cast<char*>(&keyLen), sizeof(size_t));
                std::string key(keyLen, '\0');
                if (keyLen > 0) {
                    file.read(&key[0], keyLen);
                }
                time_t timeValue;
                file.read(reinterpret_cast<char*>(&timeValue), sizeof(time_t));
                timeMap[key] = timeValue;
            }
            smp.SetMemoryLastModifiedTimeMap(timeMap);
        }
        // 版本1的文件没有时间信息，timeMap 保持为空

        // 9. 读取内存池数据
        uint8_t* poolData = smp.GetPoolData();
        file.read(reinterpret_cast<char*>(poolData), SharedMemoryPool::kPoolSize);

        return file.good();
    } catch (...) {
        return false;
    }
}
} // namespace Persistence