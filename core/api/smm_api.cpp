// Shared Memory Management API Implementation
// C API 包装层，将 C++ 的 SharedMemoryPool 类包装成 C API

#include "smm_api.h"
#include "../shared_memory_pool/shared_memory_pool.h"
#include "../persistence/persistence.h"
#include <map>
#include <string>
#include <cstring>
#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif

// 全局错误码（线程局部存储）
static thread_local SMM_ErrorCode g_last_error = SMM_SUCCESS;

// 句柄到对象的映射（使用互斥锁保护，支持多线程）
static std::map<SMM_PoolHandle, SharedMemoryPool*> g_pools;
static std::mutex g_pools_mutex;

// 辅助函数：验证句柄
static SharedMemoryPool* GetPool(SMM_PoolHandle handle) {
    if (!handle) {
        g_last_error = SMM_ERROR_INVALID_HANDLE;
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(g_pools_mutex);
    auto it = g_pools.find(handle);
    if (it == g_pools.end()) {
        g_last_error = SMM_ERROR_INVALID_HANDLE;
        return nullptr;
    }
    return it->second;
}

// 辅助函数：设置错误码
static void SetError(SMM_ErrorCode error) {
    g_last_error = error;
}

// 版本信息
SMM_ErrorCode smm_get_version(int* major, int* minor, int* patch) {
    if (!major || !minor || !patch) {
        SetError(SMM_ERROR_INVALID_PARAM);
        return SMM_ERROR_INVALID_PARAM;
    }
    *major = SMM_VERSION_MAJOR;
    *minor = SMM_VERSION_MINOR;
    *patch = SMM_VERSION_PATCH;
    SetError(SMM_SUCCESS);
    return SMM_SUCCESS;
}

// 创建内存池
SMM_PoolHandle smm_create_pool(size_t pool_size) {
    try {
        // 注意：当前实现固定使用 1GB，pool_size 参数暂时忽略
        // 后续可以扩展 SharedMemoryPool 支持自定义大小
        (void)pool_size; // 避免未使用参数警告

        SharedMemoryPool* pool = new SharedMemoryPool();
        if (!pool->Init()) {
            delete pool;
            SetError(SMM_ERROR_OUT_OF_MEMORY);
            return nullptr;
        }

        SMM_PoolHandle handle = static_cast<SMM_PoolHandle>(pool);
        std::lock_guard<std::mutex> lock(g_pools_mutex);
        g_pools[handle] = pool;

        SetError(SMM_SUCCESS);
        return handle;
    } catch (const std::bad_alloc&) {
        SetError(SMM_ERROR_OUT_OF_MEMORY);
        return nullptr;
    } catch (...) {
        SetError(SMM_ERROR_UNKNOWN);
        return nullptr;
    }
}

// 销毁内存池
SMM_ErrorCode smm_destroy_pool(SMM_PoolHandle pool) {
    if (!pool) {
        SetError(SMM_ERROR_INVALID_PARAM);
        return SMM_ERROR_INVALID_PARAM;
    }

    std::lock_guard<std::mutex> lock(g_pools_mutex);
    auto it = g_pools.find(pool);
    if (it == g_pools.end()) {
        SetError(SMM_ERROR_INVALID_HANDLE);
        return SMM_ERROR_INVALID_HANDLE;
    }

    delete it->second;
    g_pools.erase(it);

    SetError(SMM_SUCCESS);
    return SMM_SUCCESS;
}

// 重置内存池
SMM_ErrorCode smm_reset_pool(SMM_PoolHandle pool) {
    SharedMemoryPool* smp = GetPool(pool);
    if (!smp) {
        return g_last_error;
    }

    try {
        smp->Reset();
        SetError(SMM_SUCCESS);
        return SMM_SUCCESS;
    } catch (...) {
        SetError(SMM_ERROR_UNKNOWN);
        return SMM_ERROR_UNKNOWN;
    }
}

// 分配内存
SMM_ErrorCode smm_alloc(SMM_PoolHandle pool, const char* description, const void* data,
                        size_t data_size, char* memory_id_out, size_t memory_id_size) {
    SharedMemoryPool* smp = GetPool(pool);
    if (!smp) {
        return g_last_error;
    }

    if (!description || !data || !memory_id_out) {
        SetError(SMM_ERROR_INVALID_PARAM);
        return SMM_ERROR_INVALID_PARAM;
    }

    if (data_size == 0) {
        SetError(SMM_ERROR_INVALID_PARAM);
        return SMM_ERROR_INVALID_PARAM;
    }

    try {
        // 生成 memory_id
        std::string memory_id = smp->GenerateNextMemoryId();

        // 分配内存
        int result = smp->AllocateBlock(memory_id, std::string(description), data, data_size);
        if (result < 0) {
            SetError(SMM_ERROR_OUT_OF_MEMORY);
            return SMM_ERROR_OUT_OF_MEMORY;
        }

        // 复制 memory_id 到输出缓冲区
        if (memory_id.size() >= memory_id_size) {
            SetError(SMM_ERROR_INVALID_PARAM);
            return SMM_ERROR_INVALID_PARAM;
        }
        std::strncpy(memory_id_out, memory_id.c_str(), memory_id_size - 1);
        memory_id_out[memory_id_size - 1] = '\0';

        SetError(SMM_SUCCESS);
        return SMM_SUCCESS;
    } catch (const std::bad_alloc&) {
        SetError(SMM_ERROR_OUT_OF_MEMORY);
        return SMM_ERROR_OUT_OF_MEMORY;
    } catch (...) {
        SetError(SMM_ERROR_UNKNOWN);
        return SMM_ERROR_UNKNOWN;
    }
}

// 释放内存
SMM_ErrorCode smm_free(SMM_PoolHandle pool, const char* memory_id) {
    SharedMemoryPool* smp = GetPool(pool);
    if (!smp) {
        return g_last_error;
    }

    if (!memory_id) {
        SetError(SMM_ERROR_INVALID_PARAM);
        return SMM_ERROR_INVALID_PARAM;
    }

    try {
        if (!smp->FreeByMemoryId(std::string(memory_id))) {
            SetError(SMM_ERROR_NOT_FOUND);
            return SMM_ERROR_NOT_FOUND;
        }

        SetError(SMM_SUCCESS);
        return SMM_SUCCESS;
    } catch (...) {
        SetError(SMM_ERROR_UNKNOWN);
        return SMM_ERROR_UNKNOWN;
    }
}

// 更新内存
SMM_ErrorCode smm_update(SMM_PoolHandle pool, const char* memory_id, const void* new_data,
                         size_t new_data_size) {
    SharedMemoryPool* smp = GetPool(pool);
    if (!smp) {
        return g_last_error;
    }

    if (!memory_id || !new_data) {
        SetError(SMM_ERROR_INVALID_PARAM);
        return SMM_ERROR_INVALID_PARAM;
    }

    if (new_data_size == 0) {
        SetError(SMM_ERROR_INVALID_PARAM);
        return SMM_ERROR_INVALID_PARAM;
    }

    try {
        std::string mem_id(memory_id);

        // 检查 Memory ID 是否存在
        const auto& memoryInfo = smp->GetMemoryInfo();
        auto it = memoryInfo.find(mem_id);
        if (it == memoryInfo.end()) {
            SetError(SMM_ERROR_NOT_FOUND);
            return SMM_ERROR_NOT_FOUND;
        }

        // 获取当前描述
        const auto& meta = smp->GetMeta(it->second.first);
        std::string description = meta.description;

        // 释放旧内存
        smp->FreeByMemoryId(mem_id);

        // 分配新内存
        int result = smp->AllocateBlock(mem_id, description, new_data, new_data_size);
        if (result < 0) {
            SetError(SMM_ERROR_OUT_OF_MEMORY);
            return SMM_ERROR_OUT_OF_MEMORY;
        }

        SetError(SMM_SUCCESS);
        return SMM_SUCCESS;
    } catch (const std::bad_alloc&) {
        SetError(SMM_ERROR_OUT_OF_MEMORY);
        return SMM_ERROR_OUT_OF_MEMORY;
    } catch (...) {
        SetError(SMM_ERROR_UNKNOWN);
        return SMM_ERROR_UNKNOWN;
    }
}

// 读取内存
SMM_ErrorCode smm_read(SMM_PoolHandle pool, const char* memory_id, void* buffer, size_t buffer_size,
                       size_t* actual_size) {
    SharedMemoryPool* smp = GetPool(pool);
    if (!smp) {
        return g_last_error;
    }

    if (!memory_id || !buffer || !actual_size) {
        SetError(SMM_ERROR_INVALID_PARAM);
        return SMM_ERROR_INVALID_PARAM;
    }

    try {
        std::string content = smp->GetMemoryContentAsString(std::string(memory_id));
        if (content.empty()) {
            // 检查是否存在
            const auto& memoryInfo = smp->GetMemoryInfo();
            if (memoryInfo.find(std::string(memory_id)) == memoryInfo.end()) {
                SetError(SMM_ERROR_NOT_FOUND);
                return SMM_ERROR_NOT_FOUND;
            }
            // 存在但内容为空
            *actual_size = 0;
            SetError(SMM_SUCCESS);
            return SMM_SUCCESS;
        }

        // 复制内容到缓冲区
        size_t copy_size = (content.size() < buffer_size) ? content.size() : buffer_size;
        std::memcpy(buffer, content.data(), copy_size);
        *actual_size = copy_size;

        // 如果缓冲区太小，仍然返回成功，但 actual_size 会小于实际大小
        SetError(SMM_SUCCESS);
        return SMM_SUCCESS;
    } catch (...) {
        SetError(SMM_ERROR_UNKNOWN);
        return SMM_ERROR_UNKNOWN;
    }
}

// 获取状态信息
SMM_ErrorCode smm_get_status(SMM_PoolHandle pool, SMM_StatusInfo* status_out) {
    SharedMemoryPool* smp = GetPool(pool);
    if (!smp) {
        return g_last_error;
    }

    if (!status_out) {
        SetError(SMM_ERROR_INVALID_PARAM);
        return SMM_ERROR_INVALID_PARAM;
    }

    try {
        const auto& memoryInfo = smp->GetMemoryInfo();
        size_t total_blocks = SharedMemoryPool::kBlockCount;
        size_t free_blocks = smp->GetFreeBlockCount();
        size_t used_blocks = total_blocks - free_blocks;

        status_out->total_blocks = total_blocks;
        status_out->free_blocks = free_blocks;
        status_out->used_blocks = used_blocks;
        status_out->allocated_count = memoryInfo.size();
        status_out->pool_size = SharedMemoryPool::kPoolSize;
        status_out->block_size = SharedMemoryPool::kBlockSize;

        SetError(SMM_SUCCESS);
        return SMM_SUCCESS;
    } catch (...) {
        SetError(SMM_ERROR_UNKNOWN);
        return SMM_ERROR_UNKNOWN;
    }
}

// 获取内存信息
SMM_ErrorCode smm_get_memory_info(SMM_PoolHandle pool, const char* memory_id,
                                  SMM_MemoryInfo* info_out) {
    SharedMemoryPool* smp = GetPool(pool);
    if (!smp) {
        return g_last_error;
    }

    if (!memory_id || !info_out) {
        SetError(SMM_ERROR_INVALID_PARAM);
        return SMM_ERROR_INVALID_PARAM;
    }

    try {
        std::string mem_id(memory_id);
        const auto& memoryInfo = smp->GetMemoryInfo();
        auto it = memoryInfo.find(mem_id);
        if (it == memoryInfo.end()) {
            SetError(SMM_ERROR_NOT_FOUND);
            return SMM_ERROR_NOT_FOUND;
        }

        // 复制 memory_id
        std::strncpy(info_out->memory_id, mem_id.c_str(), sizeof(info_out->memory_id) - 1);
        info_out->memory_id[sizeof(info_out->memory_id) - 1] = '\0';

        // 获取描述
        const auto& meta = smp->GetMeta(it->second.first);
        std::strncpy(info_out->description, meta.description.c_str(),
                     sizeof(info_out->description) - 1);
        info_out->description[sizeof(info_out->description) - 1] = '\0';

        // 设置块信息
        info_out->start_block = it->second.first;
        info_out->block_count = it->second.second;
        info_out->data_size = it->second.second * SharedMemoryPool::kBlockSize;

        // 获取最后修改时间
        info_out->last_modified = smp->GetMemoryLastModifiedTime(mem_id);

        SetError(SMM_SUCCESS);
        return SMM_SUCCESS;
    } catch (...) {
        SetError(SMM_ERROR_UNKNOWN);
        return SMM_ERROR_UNKNOWN;
    }
}

// 紧凑内存
SMM_ErrorCode smm_compact(SMM_PoolHandle pool) {
    SharedMemoryPool* smp = GetPool(pool);
    if (!smp) {
        return g_last_error;
    }

    try {
        smp->Compact();
        SetError(SMM_SUCCESS);
        return SMM_SUCCESS;
    } catch (...) {
        SetError(SMM_ERROR_UNKNOWN);
        return SMM_ERROR_UNKNOWN;
    }
}

// 保存到文件
SMM_ErrorCode smm_save(SMM_PoolHandle pool, const char* filename) {
    SharedMemoryPool* smp = GetPool(pool);
    if (!smp) {
        return g_last_error;
    }

    if (!filename) {
        SetError(SMM_ERROR_INVALID_PARAM);
        return SMM_ERROR_INVALID_PARAM;
    }

    try {
        if (Persistence::Save(*smp, std::string(filename))) {
            SetError(SMM_SUCCESS);
            return SMM_SUCCESS;
        } else {
            SetError(SMM_ERROR_IO_FAILED);
            return SMM_ERROR_IO_FAILED;
        }
    } catch (...) {
        SetError(SMM_ERROR_IO_FAILED);
        return SMM_ERROR_IO_FAILED;
    }
}

// 从文件加载
SMM_ErrorCode smm_load(SMM_PoolHandle pool, const char* filename) {
    SharedMemoryPool* smp = GetPool(pool);
    if (!smp) {
        return g_last_error;
    }

    if (!filename) {
        SetError(SMM_ERROR_INVALID_PARAM);
        return SMM_ERROR_INVALID_PARAM;
    }

    try {
        if (Persistence::Load(*smp, std::string(filename))) {
            SetError(SMM_SUCCESS);
            return SMM_SUCCESS;
        } else {
            SetError(SMM_ERROR_IO_FAILED);
            return SMM_ERROR_IO_FAILED;
        }
    } catch (...) {
        SetError(SMM_ERROR_IO_FAILED);
        return SMM_ERROR_IO_FAILED;
    }
}

// 获取最后的错误码
SMM_ErrorCode smm_get_last_error(void) {
    return g_last_error;
}

// 获取错误字符串
const char* smm_get_error_string(SMM_ErrorCode error) {
    switch (error) {
    case SMM_SUCCESS:
        return "Success";
    case SMM_ERROR_INVALID_HANDLE:
        return "Invalid handle";
    case SMM_ERROR_INVALID_PARAM:
        return "Invalid parameter";
    case SMM_ERROR_OUT_OF_MEMORY:
        return "Out of memory";
    case SMM_ERROR_NOT_FOUND:
        return "Not found";
    case SMM_ERROR_ALREADY_EXISTS:
        return "Already exists";
    case SMM_ERROR_IO_FAILED:
        return "I/O operation failed";
    case SMM_ERROR_UNKNOWN:
    default:
        return "Unknown error";
    }
}

#ifdef __cplusplus
}
#endif
