// Shared Memory Management API - 基础使用示例
// 演示如何使用 C API 进行基本的内存管理操作

#include "smm_api.h"
#include <iostream>
#include <cstring>

int main() {
    std::cout << "=== Shared Memory Management API 示例 ===\n\n";

    // 1. 创建内存池
    std::cout << "创建内存池...\n";
    SMM_PoolHandle pool = smm_create_pool(1024 * 1024 * 1024); // 1GB
    if (!pool) {
        std::cerr << "创建内存池失败: " << smm_get_error_string(smm_get_last_error()) << "\n";
        return 1;
    }
    std::cout << "内存池创建成功\n\n";

    // 2. 分配内存
    std::cout << "分配内存...\n";
    const char* test_data = "Hello, Shared Memory Management!";
    char memory_id[64];
    SMM_ErrorCode err =
        smm_alloc(pool, "测试数据", test_data, strlen(test_data), memory_id, sizeof(memory_id));
    if (err != SMM_SUCCESS) {
        std::cerr << "分配内存失败: " << smm_get_error_string(err) << "\n";
        smm_destroy_pool(pool);
        return 1;
    }
    std::cout << "内存分配成功，Memory ID: " << memory_id << "\n\n";

    // 3. 读取内存
    std::cout << "读取内存...\n";
    char buffer[256];
    size_t actual_size;
    err = smm_read(pool, memory_id, buffer, sizeof(buffer), &actual_size);
    if (err == SMM_SUCCESS) {
        std::cout << "读取成功，内容: " << std::string(buffer, actual_size) << "\n";
        std::cout << "实际大小: " << actual_size << " 字节\n\n";
    } else {
        std::cerr << "读取失败: " << smm_get_error_string(err) << "\n";
    }

    // 4. 获取状态信息
    std::cout << "获取内存池状态...\n";
    SMM_StatusInfo status;
    err = smm_get_status(pool, &status);
    if (err == SMM_SUCCESS) {
        std::cout << "总块数: " << status.total_blocks << "\n";
        std::cout << "空闲块数: " << status.free_blocks << "\n";
        std::cout << "已使用块数: " << status.used_blocks << "\n";
        std::cout << "已分配项数: " << status.allocated_count << "\n";
        std::cout << "内存池大小: " << status.pool_size << " 字节\n";
        std::cout << "块大小: " << status.block_size << " 字节\n\n";
    }

    // 5. 获取内存信息
    std::cout << "获取内存信息...\n";
    SMM_MemoryInfo info;
    err = smm_get_memory_info(pool, memory_id, &info);
    if (err == SMM_SUCCESS) {
        std::cout << "Memory ID: " << info.memory_id << "\n";
        std::cout << "描述: " << info.description << "\n";
        std::cout << "起始块: " << info.start_block << "\n";
        std::cout << "块数量: " << info.block_count << "\n";
        std::cout << "数据大小: " << info.data_size << " 字节\n";
        std::cout << "最后修改时间: " << info.last_modified << "\n\n";
    }

    // 6. 更新内存
    std::cout << "更新内存...\n";
    const char* new_data = "Updated content!";
    err = smm_update(pool, memory_id, new_data, strlen(new_data));
    if (err == SMM_SUCCESS) {
        std::cout << "更新成功\n\n";

        // 再次读取验证
        err = smm_read(pool, memory_id, buffer, sizeof(buffer), &actual_size);
        if (err == SMM_SUCCESS) {
            std::cout << "更新后的内容: " << std::string(buffer, actual_size) << "\n\n";
        }
    } else {
        std::cerr << "更新失败: " << smm_get_error_string(err) << "\n";
    }

    // 7. 释放内存
    std::cout << "释放内存...\n";
    err = smm_free(pool, memory_id);
    if (err == SMM_SUCCESS) {
        std::cout << "释放成功\n\n";
    } else {
        std::cerr << "释放失败: " << smm_get_error_string(err) << "\n";
    }

    // 8. 销毁内存池
    std::cout << "销毁内存池...\n";
    err = smm_destroy_pool(pool);
    if (err == SMM_SUCCESS) {
        std::cout << "销毁成功\n";
    } else {
        std::cerr << "销毁失败: " << smm_get_error_string(err) << "\n";
    }

    return 0;
}
