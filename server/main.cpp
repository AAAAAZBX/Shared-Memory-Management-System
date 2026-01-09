#include "command/commands.h"
#include "shared_memory_pool/shared_memory_pool.h"
#include "persistence/persistence.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <csignal>
#include <cstdlib>

static SharedMemoryPool* g_smp = nullptr;

void SignalHandler(int signal) {
    if (g_smp != nullptr) {
        std::cerr << "\n\nReceived signal " << signal << ", saving data...\n";
        if (Persistence::Save(*g_smp)) {
            std::cerr << "Data saved successfully.\n";
        } else {
            std::cerr << "Failed to save data!\n";
        }
    }
    std::exit(signal);
}

static std::vector<std::string> Split(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> t;
    std::string x;
    while (iss >> x)
        t.push_back(x);
    return t;
}

int main() {
    // 确保输出立即显示
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    SharedMemoryPool smp;
    g_smp = &smp;

    // 注册信号处理器
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
#ifdef _WIN32
    std::signal(SIGBREAK, SignalHandler);
#endif

    // 先初始化内存池（分配内存空间）
    if (!smp.Init()) {
        std::cerr << "Failed to initialize SharedMemoryPool.\n";
        std::cerr.flush();
        return 1;
    }

    // 尝试加载之前保存的数据
    if (Persistence::Load(smp)) {
        std::cout << "Loaded previous state from " << Persistence::kDefaultFile << "\n";
    } else {
        std::cout << "Initialized new memory pool.\n";
    }

    std::cout << "RemoteMem Server (toy) - type 'help'\n";
    std::cout.flush();

    std::string line;

    // 检查标准输入是否可用
    if (!std::cin.good()) {
        std::cerr << "Warning: Standard input is not available.\n";
        std::cerr.flush();
    }

    while (true) {
        std::cout << "server> ";
        std::cout.flush();

        if (!std::getline(std::cin, line)) {
            // 如果 getline 失败，可能是 EOF 或输入流关闭
            if (std::cin.eof()) {
                std::cerr << "\nEOF reached, saving data...\n";
            } else if (std::cin.fail()) {
                std::cerr << "\nInput stream error, saving data...\n";
            }
            if (Persistence::Save(smp)) {
                std::cerr << "Data saved successfully.\n";
            } else {
                std::cerr << "Failed to save data!\n";
            }
            break;
        }

        auto tokens = Split(line);
        if (tokens.empty()) {
            continue;
        }

        if (tokens[0] == "quit" || tokens[0] == "exit") {
            std::cout << "Saving data...\n";
            if (Persistence::Save(smp)) {
                std::cout << "Data saved successfully.\n";
            } else {
                std::cerr << "Failed to save data!\n";
            }
            std::cout << "bye\n";
            break;
        }

        HandleCommand(tokens, smp);
    }

    g_smp = nullptr;
    return 0;
}
