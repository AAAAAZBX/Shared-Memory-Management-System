#include "commands.h"
#include "../../core/shared_memory_pool/shared_memory_pool.h"
#include "../../core/persistence/persistence.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <fstream>
#include <climits>
#include <vector>
#include <filesystem>
#include <windows.h>

static const std::vector<CommandSpec> kCmds = {
    // 帮助命令
    {"help", "Show help information", "help [command]", {"help", "help blocks", "help allocs"}},

    // 状态命令
    {"status",
     "Show server status (use --memory or --block)",
     "status [--memory|--block]",
     {"status", "status --memory", "status --block"}},

    // 信息命令
    {"info", "Show comprehensive system information", "info", {"info"}},

    // 分配命令
    {"alloc",
     "Allocate memory with description and content (supports file upload for .txt files)",
     "alloc \"<description>\" \"<content>\" or alloc \"<description>\" @<filepath>",
     {"alloc \"User Data\" \"Hello World\"", "alloc \"Config\" \"key=value\"",
      "alloc \"Document\" @file.txt", "alloc \"Document\" @C:\\Users\\file.txt"}},

    // 读取命令
    {"read",
     "Show content by memory ID",
     "read <memory_id>",
     {"read memory_00001", "read memory_00002"}},

    // 释放命令
    {"free",
     "Free memory by memory ID",
     "free <memory_id>",
     {"free memory_00001", "free memory_00002"}},
    {"delete",
     "Delete memory by memory ID (alias for free)",
     "delete <memory_id>",
     {"delete memory_00001", "delete memory_00002"}},

    // 更新命令
    {"update",
     "Update content for a memory ID",
     "update <memory_id> \"<new_content>\"",
     {"update memory_00001 \"New Content\"", "update memory_00002 \"Updated\""}},

    // 紧凑命令
    {"compact", "Compact memory pool (merge free blocks)", "compact", {"compact"}},

    // 执行文件命令
    {"exec",
     "Execute commands from a file",
     "exec <filename>",
     {"exec sample.txt", "exec commands.txt"}},

    // 重置命令
    {"reset", "Reset memory pool (requires password confirmation)", "reset", {"reset"}},

    // 退出命令
    {"quit", "Exit server console", "quit", {"quit"}}};

// 辅助函数：分割字符串
static std::vector<std::string> Split(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// 查找命令
static const CommandSpec* Find(const std::string& name) {
    for (const auto& c : kCmds) {
        if (c.name == name)
            return &c;
    }
    return nullptr;
}

// 解析带双引号的字符串
static std::string ParseQuotedString(const std::vector<std::string>& tokens, size_t startIdx) {
    std::string result;
    bool inQuotes = false;

    for (size_t i = startIdx; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];

        if (token.empty()) {
            if (inQuotes) {
                result += " "; // 空字符串在引号内，添加空格
            }
            continue;
        }

        if (!inQuotes) {
            // 检查是否以双引号开始
            if (token.front() == '"') {
                inQuotes = true;
                if (token.back() == '"' && token.size() > 1) {
                    // 整个字符串在一对引号中
                    return token.substr(1, token.size() - 2);
                }
                result = token.substr(1); // 去掉开头的引号
            } else {
                return ""; // 格式错误
            }
        } else {
            // 在引号内
            if (token.back() == '"') {
                // 找到结束引号
                result += " " + token.substr(0, token.size() - 1);
                return result;
            } else {
                result += " " + token;
            }
        }
    }

    return inQuotes ? result : ""; // 如果还在引号内，返回已解析的部分
}

// 读取文件内容（支持 .txt 文件）
// filepath 可以是绝对路径（如 "C:\Users\file.txt"）或相对路径（如 "file.txt"）
// 支持 UTF-8 编码的中文路径
static bool ReadFileContent(const std::string& filepath, std::string& content) {
    // 检查文件扩展名是否为 .txt
    if (filepath.length() < 4 || filepath.substr(filepath.length() - 4) != ".txt") {
        return false;
    }

    // 将 UTF-8 编码的路径转换为 UTF-16（Windows 文件系统使用 UTF-16）
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, filepath.c_str(), -1, nullptr, 0);
    if (wideSize <= 0) {
        return false;
    }
    std::vector<wchar_t> widePath(wideSize);
    if (MultiByteToWideChar(CP_UTF8, 0, filepath.c_str(), -1, widePath.data(), wideSize) <= 0) {
        return false;
    }

    // 使用宽字符路径打开文件
    std::filesystem::path path(widePath.data());
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // 检查文件大小是否超过内存池限制（预留一些空间）
    constexpr size_t kMaxFileSize = SharedMemoryPool::kPoolSize - (1024 * 1024); // 预留1MB
    if (fileSize > kMaxFileSize) {
        file.close();
        return false;
    }

    // 读取文件内容
    content.resize(fileSize);
    if (fileSize > 0) {
        file.read(&content[0], static_cast<std::streamsize>(fileSize));

        // 检查并移除 UTF-8 BOM（如果存在）
        // UTF-8 BOM: EF BB BF (3 bytes)
        if (fileSize >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
            static_cast<unsigned char>(content[1]) == 0xBB &&
            static_cast<unsigned char>(content[2]) == 0xBF) {
            // 移除 BOM
            content = content.substr(3);
        }
    }
    file.close();

    return true;
}

// 检查字符串是否是文件路径（以 @ 开头或 .txt 结尾）
static bool IsFilePath(const std::string& str) {
    // 检查是否以 @ 开头（如 @file.txt）
    if (str.length() > 0 && str[0] == '@') {
        return true;
    }
    // 检查是否以 .txt 结尾
    if (str.length() >= 4 && str.substr(str.length() - 4) == ".txt") {
        return true;
    }
    return false;
}

// 从文件路径字符串中提取实际路径（去掉 @ 前缀）
// 支持绝对路径（如 @"C:\Users\file.txt" 或 @C:\Users\file.txt）和相对路径（如 @file.txt）
static std::string ExtractFilePath(const std::string& str) {
    if (str.length() > 0 && str[0] == '@') {
        std::string path = str.substr(1);
        // 如果路径在引号内（ParseQuotedString 可能已经处理了，但为了安全还是检查一下）
        if (path.length() >= 2 && path.front() == '"' && path.back() == '"') {
            return path.substr(1, path.length() - 2);
        }
        return path;
    }
    return str;
}

// 计算字符串的显示宽度（中文字符算2个宽度，ASCII字符算1个宽度）
static size_t GetDisplayWidth(const std::string& str) {
    size_t width = 0;
    for (size_t i = 0; i < str.length();) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (c < 0x80) {
            // ASCII 字符，宽度为1
            width += 1;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // UTF-8 2字节字符（通常是中文等）
            width += 2;
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // UTF-8 3字节字符（通常是中文等）
            width += 2;
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // UTF-8 4字节字符
            width += 2;
            i += 4;
        } else {
            // 无效字符，跳过
            i += 1;
        }
    }
    return width;
}

// 将字符串填充到指定显示宽度（右侧填充空格）
static std::string PadToDisplayWidth(const std::string& str, size_t targetWidth) {
    size_t currentWidth = GetDisplayWidth(str);
    if (currentWidth >= targetWidth) {
        return str;
    }
    return str + std::string(targetWidth - currentWidth, ' ');
}

// 将字符串截断到指定显示宽度（如果超出则添加...）
static std::string TruncateToDisplayWidth(const std::string& str, size_t targetWidth) {
    if (GetDisplayWidth(str) <= targetWidth) {
        return str;
    }

    // 需要截断，找到合适的截断位置
    size_t currentWidth = 0;
    size_t i = 0;
    while (i < str.length() && currentWidth + 3 <= targetWidth) { // 3是"..."的宽度
        unsigned char c = static_cast<unsigned char>(str[i]);
        size_t charWidth = 1;
        size_t charLen = 1;

        if (c < 0x80) {
            charWidth = 1;
            charLen = 1;
        } else if ((c & 0xE0) == 0xC0) {
            charWidth = 2;
            charLen = 2;
        } else if ((c & 0xF0) == 0xE0) {
            charWidth = 2;
            charLen = 3;
        } else if ((c & 0xF8) == 0xF0) {
            charWidth = 2;
            charLen = 4;
        }

        if (currentWidth + charWidth + 3 > targetWidth) {
            break;
        }

        currentWidth += charWidth;
        i += charLen;
    }

    return str.substr(0, i) + "...";
}

// 打印帮助概览
void PrintHelpOverview() {
    std::cout << "Commands:\n";
    for (const auto& c : kCmds) {
        std::cout << "  " << std::left << std::setw(10) << c.name << c.summary << "\n";
    }
    std::cout << "\nType: help <command> for details.\n";
}

// 打印帮助命令
void PrintHelpCommand(const std::string& cmd) {
    auto* c = Find(cmd);
    if (!c) {
        std::cout << "Unknown command: " << cmd << "\n";
        std::cout << "Type 'help' to list commands.\n";
        return;
    }

    std::cout << c->name << " - " << c->summary << "\n";
    std::cout << "Usage:\n  " << c->usage << "\n";
    if (!c->examples.empty()) {
        std::cout << "Examples:\n";
        for (auto& ex : c->examples)
            std::cout << "  " << ex << "\n";
    }
}

// 处理命令
void HandleCommand(const std::vector<std::string>& tokens, SharedMemoryPool& smp) {
    const std::string& cmd = tokens[0];

    // help 命令
    if (cmd == "help") {
        if (tokens.size() == 1)
            PrintHelpOverview();
        else
            PrintHelpCommand(tokens[1]);
        return;
    }

    // status 命令
    else if (cmd == "status") {
        // 默认显示 memory，或者根据参数选择
        std::string mode = "--memory"; // 默认模式
        if (tokens.size() > 1) {
            mode = tokens[1];
        }

        if (mode == "--memory") {
            // 原来的 status 命令内容
            std::cout << "Memory Pool Status:\n";
            std::cout << "|    MemoryID    |    Description    |      Bytes      |                 "
                         "    range                    |    Last "
                         "Modified    |\n";
            std::cout << "|----------------|-------------------|-----------------|-----------------"
                         "-----------------------------|----------"
                         "-----------|\n";

            // 使用指针避免复制数据，按起始 block 排序
            const auto& memoryInfo = smp.GetMemoryInfo();
            std::vector<const std::pair<const std::string, std::pair<size_t, size_t>>*>
                sortedEntries;
            sortedEntries.reserve(memoryInfo.size());
            for (const auto& entry : memoryInfo) {
                sortedEntries.push_back(&entry);
            }
            // 按照起始 block ID 排序
            std::sort(sortedEntries.begin(), sortedEntries.end(), [](const auto* a, const auto* b) {
                return a->second.first < b->second.first;
            });

            for (const auto* entry : sortedEntries) {
                size_t blockCount = entry->second.second;
                size_t totalBytes = blockCount * SharedMemoryPool::kBlockSize;
                size_t totalKB = totalBytes / 1024;

                std::ostringstream rangeStream;
                rangeStream << "block_" << std::setfill('0') << std::setw(3) << entry->second.first
                            << " - "
                            << "block_" << std::setfill('0') << std::setw(3)
                            << (entry->second.first + entry->second.second - 1) << "(" << blockCount
                            << " blocks, " << totalKB << "KB)";
                std::string rangeStr = rangeStream.str();
                const auto& meta = smp.GetMeta(entry->second.first);
                std::string description = meta.description.empty() ? "-" : meta.description;

                // 计算实际字节数（通过读取内容）
                std::string content = smp.GetMemoryContentAsString(entry->first);
                size_t bytes = content.size();

                // 格式化字节数（添加千位分隔符）
                std::ostringstream bytesStream;
                bytesStream << bytes;
                std::string bytesStr = bytesStream.str();

                // 使用显示宽度进行截断和填充（支持中文）
                std::string memoryId = PadToDisplayWidth(entry->first, 14);
                description = TruncateToDisplayWidth(description, 17);
                description = PadToDisplayWidth(description, 17);
                std::string bytesFormatted = PadToDisplayWidth(bytesStr, 15);
                std::string range = PadToDisplayWidth(rangeStr, 44);
                std::string lastModified =
                    PadToDisplayWidth(smp.GetMemoryLastModifiedTimeString(entry->first), 19);

                std::cout << "| " << memoryId << " | " << description << " | " << bytesFormatted
                          << " | " << range << " | " << lastModified << " |\n";
            }
        } else if (mode == "--block") {
            // 原来的 blocks 命令内容
            std::cout << "Block Pool Status:\n";
            std::cout
                << "|     ID    |    MemoryID    |    Description    |    Last Modified    |\n";
            std::cout
                << "|-----------|----------------|-------------------|---------------------|\n";
            bool hasUsedBlocks = false;
            for (size_t i = 0; i < SharedMemoryPool::kBlockCount; i++) {
                const auto& meta = smp.GetMeta(i);

                // 只显示已使用的块，跳过空的块
                if (!meta.used || meta.memory_id.empty()) {
                    continue;
                }

                hasUsedBlocks = true;
                std::ostringstream oss;
                oss << "block_" << std::setfill('0') << std::setw(3) << i << std::setfill(' ');

                std::string blockIdStr = oss.str();
                std::string memoryId = meta.memory_id;
                std::string description = meta.description.empty() ? "-" : meta.description;

                // 使用显示宽度进行截断和填充（支持中文）
                std::string blockId = PadToDisplayWidth(blockIdStr, 9);
                memoryId = PadToDisplayWidth(memoryId, 14);
                description = TruncateToDisplayWidth(description, 17);
                description = PadToDisplayWidth(description, 17);
                std::string lastModified =
                    PadToDisplayWidth(smp.GetMemoryLastModifiedTimeString(meta.memory_id), 19);

                std::cout << "| " << blockId << " | " << memoryId << " | " << description << " | "
                          << lastModified << " |\n";
            }

            if (!hasUsedBlocks) {
                std::cout
                    << "| No allocated blocks                                                  |\n";
            }
        } else {
            std::cout << "Unknown status mode: " << mode << "\n";
            std::cout << "Usage: status [--memory|--block]\n";
        }
        return;
    }

    // info 命令
    else if (cmd == "info") {
        std::cout << "\n";
        std::cout << "==============================================================\n";
        std::cout << "          Shared Memory Pool System Info\n";
        std::cout << "==============================================================\n";
        std::cout << "\n";

        // 1. 内存池基本信息
        std::cout << "[Memory Pool Configuration]\n";
        std::cout << "  +--------------------------------------------------------+\n";
        std::cout << "  | Total Size:     " << std::setw(10) << std::right
                  << (SharedMemoryPool::kPoolSize / 1024) << " KB (" << SharedMemoryPool::kPoolSize
                  << " bytes)\n";
        std::cout << "  | Block Size:     " << std::setw(10) << std::right
                  << (SharedMemoryPool::kBlockSize / 1024) << " KB ("
                  << SharedMemoryPool::kBlockSize << " bytes)\n";
        std::cout << "  | Total Blocks:   " << std::setw(10) << std::right
                  << SharedMemoryPool::kBlockCount << " blocks\n";
        std::cout << "  +--------------------------------------------------------+\n";
        std::cout << "\n";

        // 2. 使用情况统计
        size_t usedBlocks = SharedMemoryPool::kBlockCount - smp.GetFreeBlockCount();
        size_t freeBlocks = smp.GetFreeBlockCount();
        double usagePercent =
            (static_cast<double>(usedBlocks) / SharedMemoryPool::kBlockCount) * 100.0;
        size_t usedBytes = usedBlocks * SharedMemoryPool::kBlockSize;
        size_t freeBytes = freeBlocks * SharedMemoryPool::kBlockSize;

        std::cout << "[Usage Statistics]\n";
        std::cout << "  +--------------------------------------------------------+\n";
        std::cout << "  | Used:           " << std::setw(6) << std::right << usedBlocks
                  << " blocks (" << std::setw(8) << std::right << (usedBytes / 1024) << " KB) ["
                  << std::fixed << std::setprecision(1) << std::setw(5) << std::right
                  << usagePercent << "%]\n";
        std::cout << "  | Free:           " << std::setw(6) << std::right << freeBlocks
                  << " blocks (" << std::setw(8) << std::right << (freeBytes / 1024) << " KB) ["
                  << std::fixed << std::setprecision(1) << std::setw(5) << std::right
                  << (100.0 - usagePercent) << "%]\n";
        std::cout << "  +--------------------------------------------------------+\n";
        std::cout << "\n";

        // 3. 内存分布信息
        size_t maxContinuous = smp.GetMaxContinuousFreeBlocks();
        size_t maxContinuousBytes = maxContinuous * SharedMemoryPool::kBlockSize;
        const auto& usedMap = smp.GetUsedMap();

        // 计算碎片化程度（空闲块片段数量）
        size_t freeFragments = 0;
        bool inFreeBlock = false;
        for (size_t i = 0; i < SharedMemoryPool::kBlockCount; ++i) {
            if (!usedMap[i]) {
                if (!inFreeBlock) {
                    freeFragments++;
                    inFreeBlock = true;
                }
            } else {
                inFreeBlock = false;
            }
        }

        std::cout << "[Memory Distribution]\n";
        std::cout << "  +--------------------------------------------------------+\n";
        std::cout << "  | Max Continuous: " << std::setw(6) << std::right << maxContinuous
                  << " blocks (" << std::setw(8) << std::right << (maxContinuousBytes / 1024)
                  << " KB)\n";
        std::cout << "  | Free Fragments: " << std::setw(6) << std::right << freeFragments
                  << " fragments\n";
        if (freeFragments > 1) {
            std::cout << "  | Fragmentation:  Needs compact operation\n";
        } else {
            std::cout << "  | Fragmentation:  Memory is continuous, no compact needed\n";
        }
        std::cout << "  +--------------------------------------------------------+\n";
        std::cout << "\n";

        // 4. 内存统计
        const auto& memoryInfo = smp.GetMemoryInfo();
        size_t memoryCount = memoryInfo.size();
        size_t totalMemoryBlocks = 0;
        size_t maxMemoryBlocks = 0;
        size_t minMemoryBlocks = (memoryCount > 0) ? static_cast<size_t>(-1) : 0;
        std::string maxMemoryId, minMemoryId;

        for (const auto& entry : memoryInfo) {
            size_t blockCount = entry.second.second;
            totalMemoryBlocks += blockCount;
            if (blockCount > maxMemoryBlocks) {
                maxMemoryBlocks = blockCount;
                maxMemoryId = entry.first;
            }
            if (blockCount < minMemoryBlocks) {
                minMemoryBlocks = blockCount;
                minMemoryId = entry.first;
            }
        }

        double avgMemoryBlocks =
            (memoryCount > 0) ? (static_cast<double>(totalMemoryBlocks) / memoryCount) : 0.0;

        std::cout << "[Memory Statistics]\n";
        std::cout << "  +--------------------------------------------------------+\n";
        std::cout << "  | Active Memories: " << std::setw(6) << std::right << memoryCount
                  << " memories\n";
        if (memoryCount > 0) {
            std::cout << "  | Avg Blocks/Mem:  " << std::fixed << std::setprecision(2)
                      << std::setw(10) << std::right << avgMemoryBlocks << " blocks/memory\n";
            std::cout << "  | Max Usage:      " << std::setw(6) << std::right << maxMemoryBlocks
                      << " blocks (" << std::setw(25) << std::left << maxMemoryId << ")\n";
            if (memoryCount > 1) {
                std::cout << "  | Min Usage:      " << std::setw(6) << std::right << minMemoryBlocks
                          << " blocks (" << std::setw(25) << std::left << minMemoryId << ")\n";
            }
        }
        std::cout << "  +--------------------------------------------------------+\n";
        std::cout << "\n";

        // 5. 持久化状态
        std::string persistenceFileName = "memory_pool.dat";
        std::ifstream persistenceFile(persistenceFileName, std::ios::binary);
        bool persistenceExists = persistenceFile.good();
        persistenceFile.close();

        std::cout << "[Persistence Status]\n";
        std::cout << "  +--------------------------------------------------------+\n";
        if (persistenceExists) {
            std::ifstream file(persistenceFileName, std::ios::binary | std::ios::ate);
            size_t fileSize = file.tellg();
            file.close();
            std::cout << "  | Persistence File: Exists (" << persistenceFileName << ")\n";
            std::cout << "  | File Size:       " << std::setw(10) << std::right << (fileSize / 1024)
                      << " KB (" << fileSize << " bytes)\n";
        } else {
            std::cout << "  | Persistence File: Not found\n";
        }
        std::cout << "  +--------------------------------------------------------+\n";
        std::cout << "\n";

        // 6. 系统建议
        std::cout << "[System Recommendations]\n";
        std::cout << "  +--------------------------------------------------------+\n";
        if (usagePercent > 90.0) {
            std::cout << "  | [WARNING] Memory usage is high, consider freeing some memory\n";
        } else if (usagePercent < 10.0) {
            std::cout << "  | [OK] Memory usage is low, sufficient space available\n";
        } else {
            std::cout << "  | [OK] Memory usage is normal\n";
        }

        if (freeFragments > 1 && freeBlocks > 0) {
            std::cout
                << "  | [WARNING] Memory fragmentation detected, consider running 'compact'\n";
        }

        if (memoryCount == 0) {
            std::cout << "  | [INFO] No active memories currently\n";
        }
        std::cout << "  +--------------------------------------------------------+\n";
        std::cout << "\n";

        return;
    }

    // alloc 命令
    else if (cmd == "alloc") {
        if (tokens.size() < 3) {
            std::cout << "Usage: alloc \"<description>\" \"<content>\" or alloc \"<description>\" "
                         "@<filepath>\n";
            std::cout << "Example: alloc \"User Data\" \"Hello World\"\n";
            std::cout << "Example: alloc \"Document\" @file.txt (relative path)\n";
            std::cout << "Example: alloc \"Document\" @C:\\Users\\file.txt (absolute path)\n";
            return;
        }

        // 解析 description（带引号的字符串）
        std::string description = ParseQuotedString(tokens, 1);

        if (description.empty()) {
            std::cout << "Error: Description must be provided and enclosed in double quotes.\n";
            std::cout << "Example: alloc \"User Data\" \"Hello World\"\n";
            return;
        }

        // 解析 content：可能是带引号的字符串，也可能是以 @ 开头的文件路径
        // 需要找到第一个以 @ 开头的 token（可能在 tokens[2] 或更后面）
        std::string content;

        // 找到第一个以 @ 开头的 token 的索引
        size_t contentStartIdx = tokens.size();
        for (size_t i = 2; i < tokens.size(); ++i) {
            if (tokens[i].length() > 0 && tokens[i][0] == '@') {
                contentStartIdx = i;
                break;
            }
        }

        if (contentStartIdx < tokens.size()) {
            std::string firstToken = tokens[contentStartIdx];

            // 检查是否是带引号的文件路径 @"path"
            if (firstToken.length() >= 2 && firstToken[0] == '@' && firstToken[1] == '"') {
                // 带引号的文件路径：@"path"
                // 手动解析，去掉 @" 前缀和结尾的 "
                if (firstToken.back() == '"' && firstToken.length() > 3) {
                    // 单个 token：@"path"
                    content = "@" + firstToken.substr(2, firstToken.length() - 3);
                } else {
                    // 跨多个 tokens，需要拼接
                    std::string pathContent = firstToken.substr(2); // 去掉 @"
                    for (size_t i = contentStartIdx + 1; i < tokens.size(); ++i) {
                        const std::string& token = tokens[i];
                        if (token.back() == '"') {
                            // 找到结束引号
                            pathContent += " " + token.substr(0, token.length() - 1);
                            break;
                        } else {
                            pathContent += " " + token;
                        }
                    }
                    content = "@" + pathContent;
                }
            } else if (firstToken.length() > 0 && firstToken[0] == '@') {
                // 不带引号的文件路径：@path
                content = firstToken;
            }
        } else {
            // 没有找到 @ 开头的 token，尝试作为普通文本内容解析（带引号）
            // 找到 description 结束后的第一个 token
            size_t contentIdx = 2;
            // 如果 description 跨多个 tokens，需要找到它的结束位置
            bool inDescQuotes = false;
            for (size_t i = 1; i < tokens.size(); ++i) {
                const std::string& token = tokens[i];
                if (!inDescQuotes && token.front() == '"') {
                    inDescQuotes = true;
                }
                if (inDescQuotes && token.back() == '"') {
                    contentIdx = i + 1;
                    break;
                }
            }
            if (contentIdx < tokens.size()) {
                content = ParseQuotedString(tokens, contentIdx);
            }
        }

        // 检查 content 是否是文件路径
        std::string actualContent;
        bool isFileUpload = false;

        if (IsFilePath(content)) {
            // 提取文件路径（去掉 @ 前缀）
            std::string filepath = ExtractFilePath(content);

            // 读取文件内容（支持绝对路径和相对路径）
            if (ReadFileContent(filepath, actualContent)) {
                isFileUpload = true;
                std::cout << "Reading file: " << filepath << " (" << actualContent.size()
                          << " bytes)\n";
            } else {
                std::cout << "Error: Failed to read file '" << filepath << "'.\n";
                std::cout << "Please check:\n";
                std::cout << "  1. File exists (supports both absolute and relative paths)\n";
                std::cout << "  2. File extension is .txt\n";
                std::cout << "  3. File size is within memory pool limit\n";
                std::cout << "Examples:\n";
                std::cout << "  Relative path: alloc \"Doc\" @file.txt\n";
                std::cout << "  Absolute path: alloc \"Doc\" @C:\\Users\\Documents\\file.txt\n";
                return;
            }
        } else {
            // 普通文本内容
            if (content.empty()) {
                std::cout << "Error: Content must be provided and enclosed in double quotes.\n";
                std::cout << "Example: alloc \"User Data\" \"Hello World\"\n";
                std::cout << "Or use file upload: alloc \"Document\" @file.txt\n";
                return;
            }
            actualContent = content;
        }

        // 自动生成 memory_id
        std::string memory_id = smp.GenerateNextMemoryId();

        // 将 content 作为数据写入内存池
        int blockId =
            smp.AllocateBlock(memory_id, description, actualContent.data(), actualContent.size());

        if (blockId >= 0) {
            std::cout << "Allocation successful. Memory ID: " << memory_id << "\n";
            std::cout << "Description: " << description << "\n";
            if (isFileUpload) {
                std::cout << "File uploaded successfully (" << actualContent.size() << " bytes)\n";
            }
            std::cout << "Content stored at block " << blockId << "\n";
        } else {
            std::cout << "Allocation failed. Insufficient memory or invalid parameters.\n";
        }
        return;
    }

    // read 命令
    else if (cmd == "read") {
        if (tokens.size() < 2) {
            std::cout << "Usage: read <memory_id>\n";
            std::cout << "Example: read memory_00001\n";
            return;
        }

        std::string memory_id = tokens[1];
        std::string content = smp.GetMemoryContentAsString(memory_id);

        if (content.empty()) {
            std::cout << "Memory ID '" << memory_id << "' not found or content is empty.\n";
            return;
        }

        // 获取内存的块信息和描述
        const auto& memoryInfo = smp.GetMemoryInfo();
        auto it = memoryInfo.find(memory_id);

        // 显示元信息
        if (it != memoryInfo.end()) {
            size_t startBlock = it->second.first;
            size_t blockCount = it->second.second;
            const auto& meta = smp.GetMeta(startBlock);
            std::cout << "Memory ID: " << memory_id << "\n";
            std::cout << "Description: " << meta.description << "\n";
            std::cout << "Blocks: " << startBlock << "-" << (startBlock + blockCount - 1) << "\n";
        }

        // 上划线（虚线）
        std::cout << "----------------------------------------\n";

        // 直接输出内容（不添加引号）
        std::cout << content;

        // 如果内容不以换行符结尾，添加换行
        if (!content.empty() && content.back() != '\n') {
            std::cout << "\n";
        }

        // 下划线（虚线）
        std::cout << "----------------------------------------\n";

        // 显示大小和修改时间
        std::cout << "Size: " << content.size() << " bytes\n";
        if (it != memoryInfo.end()) {
            std::cout << "Last Modified: " << smp.GetMemoryLastModifiedTimeString(memory_id)
                      << "\n";
        }
        return;
    }

    // free/delete 命令
    else if (cmd == "free" || cmd == "delete") {
        if (tokens.size() < 2) {
            std::cout << "Usage: " << cmd << " <memory_id>\n";
            std::cout << "Example: " << cmd << " memory_00001\n";
            return;
        }

        std::string memory_id = tokens[1];
        if (smp.FreeByMemoryId(memory_id)) {
            std::cout << "Memory freed successfully for '" << memory_id << "'\n";
        } else {
            std::cout << "Memory ID '" << memory_id << "' not found.\n";
        }
        return;
    }

    // update 命令
    else if (cmd == "update") {
        if (tokens.size() < 3) {
            std::cout << "Usage: update <memory_id> \"<new_content>\"\n";
            std::cout << "Example: update memory_00001 \"New Content\"\n";
            return;
        }

        std::string memory_id = tokens[1];
        std::string newContent = ParseQuotedString(tokens, 2);

        if (newContent.empty()) {
            std::cout
                << "Error: Invalid content format. Content must be enclosed in double quotes.\n";
            std::cout << "Example: update " << memory_id << " \"New Content\"\n";
            return;
        }

        // 检查内存ID是否存在
        const auto& memoryInfo = smp.GetMemoryInfo();
        auto it = memoryInfo.find(memory_id);
        if (it == memoryInfo.end()) {
            std::cout << "Error: Memory ID '" << memory_id << "' not found.\n";
            std::cout << "Use 'alloc' command to allocate memory first.\n";
            return;
        }

        // 获取内存当前的块信息和描述
        size_t startBlock = it->second.first;
        size_t currentBlockCount = it->second.second;
        size_t currentSize = currentBlockCount * SharedMemoryPool::kBlockSize;
        const auto& meta = smp.GetMeta(startBlock);
        std::string description = meta.description;

        // 计算新内容需要的块数
        size_t newSize = newContent.size();
        size_t requiredBlockCount =
            (newSize + SharedMemoryPool::kBlockSize - 1) / SharedMemoryPool::kBlockSize;

        // 如果新内容大小超过原分配，需要重新分配
        if (newSize > currentSize) {
            // 先释放原内存
            smp.FreeByMemoryId(memory_id);
            // 重新分配（保持相同的 memory_id 和 description）
            int blockId = smp.AllocateBlock(memory_id, description, newContent.data(), newSize);
            if (blockId >= 0) {
                std::cout << "Content updated successfully. New content stored at block " << blockId
                          << "\n";
            } else {
                std::cout << "Update failed. Insufficient memory for new content.\n";
            }
        } else {
            // 新内容大小不超过原分配，直接覆盖写入
            uint8_t* poolData = smp.GetPoolData();
            size_t bytesWritten = 0;
            // 只写入需要的块数，而不是所有 currentBlockCount 块
            for (size_t i = 0; i < requiredBlockCount; ++i) {
                size_t blockId = startBlock + i;
                size_t bytesToWrite =
                    std::min(SharedMemoryPool::kBlockSize, newSize - bytesWritten);

                // 写入新数据
                std::memcpy(poolData + blockId * SharedMemoryPool::kBlockSize,
                            newContent.data() + bytesWritten, bytesToWrite);

                // 如果块没有写满，剩余部分清零
                if (bytesToWrite < SharedMemoryPool::kBlockSize) {
                    std::memset(poolData + blockId * SharedMemoryPool::kBlockSize + bytesToWrite, 0,
                                SharedMemoryPool::kBlockSize - bytesToWrite);
                }

                bytesWritten += bytesToWrite;
            }

            // 如果新内容需要的块数少于原分配，释放后续不需要的块
            if (requiredBlockCount < currentBlockCount) {
                // 先更新内存块信息中的块数量和 free_block_count
                smp.UpdateMemoryBlockCount(memory_id, requiredBlockCount);
                // 然后清理后续块的元数据和位图（不更新 free_block_count，因为已经通过
                // UpdateMemoryBlockCount 更新了）
                for (size_t i = requiredBlockCount; i < currentBlockCount; ++i) {
                    size_t blockId = startBlock + i;
                    smp.ClearBlockMeta(blockId);
                }
            }

            // 更新最后修改时间
            smp.UpdateMemoryLastModifiedTime(memory_id);

            std::cout << "Content updated successfully.\n";
        }
        return;
    }

    // compact 命令
    else if (cmd == "compact") {
        std::cout << "Compacting memory pool...\n";
        smp.Compact();
        std::cout << "Memory pool compacted\n";
        return;
    }

    // exec 命令
    else if (cmd == "exec") {
        if (tokens.size() < 2) {
            std::cout << "Usage: exec <filename>\n";
            std::cout << "Example: exec sample.txt\n";
            return;
        }

        std::string filename = tokens[1];
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Error: Cannot open file '" << filename << "'\n";
            return;
        }

        std::string line;
        size_t lineNum = 0;
        while (std::getline(file, line)) {
            lineNum++;
            // 跳过空行和注释行（以 # 开头）
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // 去除行首尾空白
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            if (line.empty()) {
                continue;
            }

            // 显示正在执行的命令
            std::cout << "[" << lineNum << "] " << line << "\n";

            // 解析并执行命令
            auto fileTokens = Split(line);
            if (!fileTokens.empty()) {
                // 如果是 quit 命令，只退出 exec，不退出程序
                if (fileTokens[0] == "quit" || fileTokens[0] == "exit") {
                    std::cout << "Quit command in file, stopping execution.\n";
                    break;
                }
                HandleCommand(fileTokens, smp);
            }
        }

        file.close();
        std::cout << "Finished executing commands from '" << filename << "'\n";
        return;
    }

    // reset 命令
    else if (cmd == "reset") {
        std::cout
            << "WARNING: This will clear all memory data and reset metadata to default state.\n";
        std::cout << "Please enter password to confirm: ";
        std::cout.flush();

        std::string password;
        if (!std::getline(std::cin, password)) {
            std::cout << "Reset cancelled.\n";
            return;
        }

        if (password == "confirm_reset") {
            smp.Reset();
            std::cout << "Memory pool has been reset to default state.\n";
        } else {
            std::cout << "Invalid password. Reset cancelled.\n";
        }
        return;
    }

    // 未知命令
    else {
        std::cout << "Unknown command: " << cmd << "\n";
        std::cout << "Type 'help' to list commands.\n";
    }
}
