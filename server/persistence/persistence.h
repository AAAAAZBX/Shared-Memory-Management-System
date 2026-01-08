#pragma once
#include "../shared_memory_pool/shared_memory_pool.h"
#include <string>

// 持久化模块
namespace Persistence {
// 保存内存池到文件
bool Save(const SharedMemoryPool& smp, const std::string& filename = "memory_pool.dat");

// 从文件加载内存池
bool Load(SharedMemoryPool& smp, const std::string& filename = "memory_pool.dat");

// 默认文件名
static constexpr const char* kDefaultFile = "memory_pool.dat";
} // namespace Persistence
