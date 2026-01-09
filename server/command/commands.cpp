#include "commands.h"
#include "../shared_memory_pool/shared_memory_pool.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <cstring>

static const std::vector<CommandSpec> kCmds = {
    // 帮助命令
    {"help", "Show help information", "help [command]", {"help", "help blocks", "help allocs"}},

    // 状态命令
    {"status",
     "Show server status (use --memory or --block)",
     "status [--memory|--block]",
     {"status", "status --memory", "status --block"}},

    // 分配命令
    {"alloc",
     "Allocate memory for user with content",
     "alloc <user> \"<content>\"",
     {"alloc 192.168.1.100:54321 \"Hello World\"", "alloc client_1 \"string1\""}},

    // 读取命令
    {"read",
     "Show content written by a user",
     "read <user>",
     {"read 192.168.1.100:54321", "read client_1"}},

    // 释放命令
    {"free",
     "Free memory allocated by a user",
     "free <user>",
     {"free 192.168.1.100:54321", "free client_1"}},
    {"delete",
     "Delete memory allocated by a user (alias for free)",
     "delete <user>",
     {"delete 192.168.1.100:54321", "delete client_1"}},

    // 更新命令
    {"update",
     "Update content for a user",
     "update <user> \"<new_content>\"",
     {"update 192.168.1.100:54321 \"New Content\"", "update client_1 \"Updated\""}},

    // 紧凑命令
    {"compact", "Compact memory pool (merge free blocks)", "compact", {"compact"}},

    // 重置命令
    {"reset", "Reset memory pool (requires password confirmation)", "reset", {"reset"}},

    // 退出命令
    {"quit", "Exit server console", "quit", {"quit"}}};

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
            std::cout << "|          range          |    Occupied Client    |\n";
            std::cout << "|-------------------------|-----------------------|\n";

            // 使用指针避免复制数据，按起始 block 排序
            const auto& userInfo = smp.GetUserBlockInfo();
            std::vector<const std::pair<const std::string, std::pair<size_t, size_t>>*>
                sortedEntries;
            sortedEntries.reserve(userInfo.size());
            for (const auto& entry : userInfo) {
                sortedEntries.push_back(&entry);
            }
            // 按照起始 block ID 排序
            std::sort(sortedEntries.begin(), sortedEntries.end(), [](const auto* a, const auto* b) {
                return a->second.first < b->second.first;
            });

            for (const auto* entry : sortedEntries) {
                std::ostringstream rangeStream;
                rangeStream << "block_" << std::setfill('0') << std::setw(3) << entry->second.first
                            << " - "
                            << "block_" << std::setfill('0') << std::setw(3)
                            << (entry->second.first + entry->second.second - 1);
                std::string rangeStr = rangeStream.str();
                std::cout << "| " << std::left << std::setw(23) << std::setfill(' ') << rangeStr
                          << " | " << std::left << std::setw(21) << entry->first << " |\n";
            }
        } else if (mode == "--block") {
            // 原来的 blocks 命令内容
            std::cout << "Block Pool Status:\n";
            std::cout << "|     ID    |    Occupied Client    |\n";
            std::cout << "|-----------|-----------------------|\n";
            for (size_t i = 0; i < SharedMemoryPool::kBlockCount; i++) {
                const auto& meta = smp.GetMeta(i);
                std::ostringstream oss;
                oss << "block_" << std::setfill('0') << std::setw(3) << i << std::setfill(' ');
                std::string client = meta.user.empty() ? "-" : meta.user;
                std::cout << "| " << std::setw(9) << oss.str() << " | " << std::left
                          << std::setw(21) << client << " |\n";
            }
        } else {
            std::cout << "Unknown status mode: " << mode << "\n";
            std::cout << "Usage: status [--memory|--block]\n";
        }
        return;
    }

    // alloc 命令
    else if (cmd == "alloc") {
        if (tokens.size() < 3) {
            std::cout << "Usage: alloc <user> \"<content>\"\n";
            std::cout << "Example: alloc 192.168.1.100:54321 \"Hello World\"\n";
            return;
        }

        std::string user = tokens[1];
        std::string content = ParseQuotedString(tokens, 2);

        if (content.empty()) {
            std::cout
                << "Error: Invalid content format. Content must be enclosed in double quotes.\n";
            std::cout << "Example: alloc " << user << " \"Hello World\"\n";
            return;
        }

        // 将 content 作为数据写入内存池
        int blockId = smp.AllocateBlock(user, content.data(), content.size());

        if (blockId >= 0) {
            std::cout << "Allocation successful. Content stored at block " << blockId << "\n";
        } else {
            std::cout << "Allocation failed. Insufficient memory or invalid parameters.\n";
        }
        return;
    }

    // read 命令
    else if (cmd == "read") {
        if (tokens.size() < 2) {
            std::cout << "Usage: read <user>\n";
            std::cout << "Example: read 192.168.1.100:54321\n";
            return;
        }

        std::string user = tokens[1];
        std::string content = smp.GetUserContentAsString(user);

        if (content.empty()) {
            std::cout << "User '" << user << "' has no allocated memory or content is empty.\n";
            return;
        }

        // 获取用户的块信息用于显示
        const auto& userInfo = smp.GetUserBlockInfo();
        auto it = userInfo.find(user);
        if (it != userInfo.end()) {
            size_t startBlock = it->second.first;
            size_t blockCount = it->second.second;
            std::cout << "User: " << user << "\n";
            std::cout << "Blocks: " << startBlock << "-" << (startBlock + blockCount - 1) << "\n";
            std::cout << "Content: \"" << content << "\"\n";
            std::cout << "Size: " << content.size() << " bytes\n";
        } else {
            std::cout << "Content: \"" << content << "\"\n";
            std::cout << "Size: " << content.size() << " bytes\n";
        }
        return;
    }

    // free/delete 命令
    else if (cmd == "free" || cmd == "delete") {
        if (tokens.size() < 2) {
            std::cout << "Usage: " << cmd << " <user>\n";
            std::cout << "Example: " << cmd << " 192.168.1.100:54321\n";
            return;
        }

        std::string user = tokens[1];
        if (smp.FreeByUser(user)) {
            std::cout << "Memory freed successfully for user '" << user << "'\n";
        } else {
            std::cout << "User '" << user << "' has no allocated memory.\n";
        }
        return;
    }

    // update 命令
    else if (cmd == "update") {
        if (tokens.size() < 3) {
            std::cout << "Usage: update <user> \"<new_content>\"\n";
            std::cout << "Example: update 192.168.1.100:54321 \"New Content\"\n";
            return;
        }

        std::string user = tokens[1];
        std::string newContent = ParseQuotedString(tokens, 2);

        if (newContent.empty()) {
            std::cout
                << "Error: Invalid content format. Content must be enclosed in double quotes.\n";
            std::cout << "Example: update " << user << " \"New Content\"\n";
            return;
        }

        // 检查用户是否存在
        const auto& userInfo = smp.GetUserBlockInfo();
        auto it = userInfo.find(user);
        if (it == userInfo.end()) {
            std::cout << "Error: User '" << user << "' has no allocated memory.\n";
            std::cout << "Use 'alloc' command to allocate memory first.\n";
            return;
        }

        // 获取用户当前的块信息
        size_t startBlock = it->second.first;
        size_t currentBlockCount = it->second.second;
        size_t currentSize = currentBlockCount * SharedMemoryPool::kBlockSize;

        // 计算新内容需要的块数
        size_t newSize = newContent.size();
        size_t requiredBlockCount =
            (newSize + SharedMemoryPool::kBlockSize - 1) / SharedMemoryPool::kBlockSize;

        // 如果新内容大小超过原分配，需要重新分配
        if (newSize > currentSize) {
            // 先释放原内存
            smp.FreeByUser(user);
            // 重新分配
            int blockId = smp.AllocateBlock(user, newContent.data(), newSize);
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
            for (size_t i = 0; i < currentBlockCount; ++i) {
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
                if (bytesWritten >= newSize) {
                    break;
                }
            }
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
