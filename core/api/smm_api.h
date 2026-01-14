#pragma once

// Shared Memory Management API
// C API for cross-language compatibility

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// API 版本定义
#define SMM_VERSION_MAJOR 1
#define SMM_VERSION_MINOR 0
#define SMM_VERSION_PATCH 0

// 类型定义
typedef void* SMM_PoolHandle;

// 状态信息结构
typedef struct {
    size_t total_blocks;    // 总块数
    size_t free_blocks;     // 空闲块数
    size_t used_blocks;     // 已使用块数
    size_t allocated_count; // 已分配的内存项数量
    size_t pool_size;       // 内存池总大小（字节）
    size_t block_size;      // 块大小（字节）
} SMM_StatusInfo;

// 内存信息结构
typedef struct {
    char memory_id[64];    // 内存ID
    char description[256]; // 描述
    size_t start_block;    // 起始块ID
    size_t block_count;    // 块数量
    size_t data_size;      // 实际数据大小（字节）
    time_t last_modified;  // 最后修改时间（Unix时间戳）
} SMM_MemoryInfo;

// 错误码
typedef enum {
    SMM_SUCCESS = 0,
    SMM_ERROR_INVALID_HANDLE = -1,
    SMM_ERROR_INVALID_PARAM = -2,
    SMM_ERROR_OUT_OF_MEMORY = -3,
    SMM_ERROR_NOT_FOUND = -4,
    SMM_ERROR_ALREADY_EXISTS = -5,
    SMM_ERROR_IO_FAILED = -6,
    SMM_ERROR_UNKNOWN = -99
} SMM_ErrorCode;

// DLL 导出宏
#ifdef _WIN32
#ifdef SMM_BUILDING_DLL
#define SMM_API __declspec(dllexport)
#else
#define SMM_API __declspec(dllimport)
#endif
#else
// 非 Windows 平台或静态链接
#define SMM_API
#endif

// 版本信息
SMM_API SMM_ErrorCode smm_get_version(int* major, int* minor, int* patch);

// 生命周期管理
SMM_API SMM_PoolHandle smm_create_pool(size_t pool_size);
SMM_API SMM_ErrorCode smm_destroy_pool(SMM_PoolHandle pool);
SMM_API SMM_ErrorCode smm_reset_pool(SMM_PoolHandle pool);

// 内存操作
SMM_API SMM_ErrorCode smm_alloc(SMM_PoolHandle pool, const char* description, const void* data,
                                size_t data_size, char* memory_id_out, size_t memory_id_size);
SMM_API SMM_ErrorCode smm_free(SMM_PoolHandle pool, const char* memory_id);
SMM_API SMM_ErrorCode smm_update(SMM_PoolHandle pool, const char* memory_id, const void* new_data,
                                 size_t new_data_size);
SMM_API SMM_ErrorCode smm_read(SMM_PoolHandle pool, const char* memory_id, void* buffer,
                               size_t buffer_size, size_t* actual_size);

// 查询操作
SMM_API SMM_ErrorCode smm_get_status(SMM_PoolHandle pool, SMM_StatusInfo* status_out);
SMM_API SMM_ErrorCode smm_get_memory_info(SMM_PoolHandle pool, const char* memory_id,
                                          SMM_MemoryInfo* info_out);

// 紧凑操作
SMM_API SMM_ErrorCode smm_compact(SMM_PoolHandle pool);

// 持久化
SMM_API SMM_ErrorCode smm_save(SMM_PoolHandle pool, const char* filename);
SMM_API SMM_ErrorCode smm_load(SMM_PoolHandle pool, const char* filename);

// 错误处理
SMM_API SMM_ErrorCode smm_get_last_error(void);
SMM_API const char* smm_get_error_string(SMM_ErrorCode error);

#ifdef __cplusplus
}
#endif
