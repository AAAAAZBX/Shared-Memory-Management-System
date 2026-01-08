#include "commands.h"
#include "shared_memory_pool.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

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
    if (!smp.Init()) {
        std::cerr << "Failed to initialize SharedMemoryPool.\n";
        std::cerr.flush();
        return 1;
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
                std::cerr << "\nEOF reached, exiting...\n";
            } else if (std::cin.fail()) {
                std::cerr << "\nInput stream error, exiting...\n";
            }
            break;
        }

        auto tokens = Split(line);
        if (tokens.empty()) {
            continue;
        }

        if (tokens[0] == "quit" || tokens[0] == "exit") {
            std::cout << "bye\n";
            break;
        }

        HandleCommand(tokens, smp);
    }

    return 0;
}