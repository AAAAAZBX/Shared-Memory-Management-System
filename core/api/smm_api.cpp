// Shared Memory Management API Implementation
// 注意：当前为占位实现，保持原功能不变
// 后续可以逐步实现完整的 C API 包装层

#include "smm_api.h"

// 当前暂时为空实现，保持程序原功能不变
// 后续可以根据 DLL_SDK_GUIDE.md 中的指导逐步实现

#ifdef __cplusplus
extern "C" {
#endif

SMM_ErrorCode smm_get_version(int* major, int* minor, int* patch) {
    if (!major || !minor || !patch) {
        return SMM_ERROR_INVALID_PARAM;
    }
    *major = SMM_VERSION_MAJOR;
    *minor = SMM_VERSION_MINOR;
    *patch = SMM_VERSION_PATCH;
    return SMM_SUCCESS;
}

// 其他函数暂时返回未实现错误
// 后续实现时会基于 SharedMemoryPool 类进行包装

#ifdef __cplusplus
}
#endif
