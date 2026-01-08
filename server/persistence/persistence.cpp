#include "persistence.h"
#include <fstream>
#include <cstring>
#include <vector>

namespace Persistence {
// 文件格式版本号和魔数
static constexpr uint32_t kFileVersion = 1;
static constexpr uint32_t kFileMagic = 0x4D454D50; // "MEMP"

// 文件头结构
struct FileHeader {
    uint32_t magic;          // 文件魔数
    uint32_t version;        // 文件格式版本
    size_t free_block_count; // 空闲块数量
    size_t user_info_count;  // user_block_info 的数量
    uint64_t reserved[4];    // 预留字段
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
        const auto& userInfo = smp.GetUserBlockInfo();
        header.free_block_count = 0; // 需要计算，暂时设为0
        header.user_info_count = userInfo.size();

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

            size_t userLen = meta.user.size();
            file.write(reinterpret_cast<const char*>(&userLen), sizeof(size_t));
            if (userLen > 0) {
                file.write(meta.user.c_str(), userLen);
            }

            size_t contentLen = meta.content.size();
            file.write(reinterpret_cast<const char*>(&contentLen), sizeof(size_t));
            if (contentLen > 0) {
                file.write(meta.content.c_str(), contentLen);
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

        // 4. 写入 user_block_info
        for (const auto& entry : userInfo) {
            size_t keyLen = entry.first.size();
            file.write(reinterpret_cast<const char*>(&keyLen), sizeof(size_t));
            file.write(entry.first.c_str(), keyLen);

            file.write(reinterpret_cast<const char*>(&entry.second.first), sizeof(size_t));
            file.write(reinterpret_cast<const char*>(&entry.second.second), sizeof(size_t));
        }

        // 5. 写入内存池数据（需要访问内部 pool_）
        // 这里需要在 SharedMemoryPool 中添加 GetPoolData() 接口
        // 暂时注释，实际使用时需要添加接口

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

        if (header.magic != kFileMagic || header.version != kFileVersion) {
            return false;
        }

        // 2. 重置内存池
        smp.Reset();

        // 3. 读取元数据
        for (size_t i = 0; i < SharedMemoryPool::kBlockCount; ++i) {
            bool used;
            file.read(reinterpret_cast<char*>(&used), sizeof(bool));

            size_t userLen;
            file.read(reinterpret_cast<char*>(&userLen), sizeof(size_t));
            std::string user(userLen, '\0');
            if (userLen > 0) {
                file.read(&user[0], userLen);
            }

            size_t contentLen;
            file.read(reinterpret_cast<char*>(&contentLen), sizeof(size_t));
            std::string content(contentLen, '\0');
            if (contentLen > 0) {
                file.read(&content[0], contentLen);
            }

            if (used) {
                smp.SetMeta(i, user, content);
            }
        }

        // 4. 读取 used_map（跳过，已通过 SetMeta 设置）
        std::vector<uint8_t> bitsetBytes((SharedMemoryPool::kBlockCount + 7) / 8, 0);
        file.read(reinterpret_cast<char*>(bitsetBytes.data()), bitsetBytes.size());

        // 5. 读取 user_block_info（需要通过接口设置）
        // 这里需要添加接口，暂时跳过

        // 6. 读取内存池数据（需要接口）

        return file.good();
    } catch (...) {
        return false;
    }
}
} // namespace Persistence