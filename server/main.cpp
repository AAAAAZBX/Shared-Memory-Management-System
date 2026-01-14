#include "command/commands.h"
#include "../core/shared_memory_pool/shared_memory_pool.h"
#include "../core/persistence/persistence.h"
#include "network/tcp_server.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <thread>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // Windows Vista or later for inet_ntop
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
// Note: ws2_32.lib must be linked via -lws2_32 compiler flag
// #pragma comment doesn't work with MinGW

static SharedMemoryPool* g_smp = nullptr;
static TCPServer* g_tcp_server = nullptr;

void SignalHandler(int signal) {
    if (g_tcp_server != nullptr) {
        std::cerr << "\n\nStopping TCP server...\n";
        g_tcp_server->Stop();
    }
    if (g_smp != nullptr) {
        std::cerr << "Saving data...\n";
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

void printLogo() {
    std::cout << "##############################################\n";
    std::cout << "#                                            #\n";
    std::cout << "#            Shared Memory Manager           #\n";
    std::cout << "#                                            #\n";
    std::cout << "##############################################\n";
}

// 判断IP地址是否为外部可访问的地址
static bool IsExternalAccessibleIP(const std::string& ip) {
    // 排除本地回环地址
    if (ip == "127.0.0.1" || ip == "::1") {
        return false;
    }

    // 排除自动配置IP（APIPA，169.254.x.x）
    if (ip.find("169.254.") == 0) {
        return false;
    }

    // 排除多播地址
    if (ip.find("224.") == 0 || ip.find("225.") == 0 || ip.find("226.") == 0 ||
        ip.find("227.") == 0 || ip.find("228.") == 0 || ip.find("229.") == 0 ||
        ip.find("230.") == 0 || ip.find("231.") == 0 || ip.find("232.") == 0 ||
        ip.find("233.") == 0 || ip.find("234.") == 0 || ip.find("235.") == 0 ||
        ip.find("236.") == 0 || ip.find("237.") == 0 || ip.find("238.") == 0 ||
        ip.find("239.") == 0) {
        return false;
    }

    // 其他地址都是外部可访问的（包括私有IP和公网IP）
    return true;
}

// 判断IP地址是否为公网IP
static bool IsPublicIP(const std::string& ip) {
    // 私有IP地址范围：
    // 10.0.0.0 - 10.255.255.255
    // 172.16.0.0 - 172.31.255.255
    // 192.168.0.0 - 192.168.255.255

    if (ip.find("10.") == 0) {
        return false; // 私有IP
    }

    if (ip.find("172.") == 0) {
        // 检查是否为 172.16-31.x.x
        size_t dot1 = ip.find('.', 4);
        if (dot1 != std::string::npos) {
            std::string second = ip.substr(4, dot1 - 4);
            int secondNum = std::stoi(second);
            if (secondNum >= 16 && secondNum <= 31) {
                return false; // 私有IP
            }
        }
    }

    if (ip.find("192.168.") == 0) {
        return false; // 私有IP
    }

    // 其他都是公网IP（或特殊地址，但我们已经排除了）
    return true;
}

// 获取本机外部可访问的IP地址列表
static std::vector<std::string> GetExternalAccessibleIPs() {
    std::vector<std::string> publicIPs;  // 公网IP
    std::vector<std::string> privateIPs; // 私有IP（局域网）

    // Windows平台：使用Winsock获取IP地址
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return publicIPs; // 返回空列表
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        struct addrinfo* result = nullptr;
        if (getaddrinfo(hostname, nullptr, &hints, &result) == 0) {
            for (struct addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
                if (ptr->ai_family == AF_INET) {
                    struct sockaddr_in* sockaddr = (struct sockaddr_in*)ptr->ai_addr;
                    const char* ipStr = inet_ntoa(sockaddr->sin_addr);
                    if (ipStr != nullptr) {
                        std::string ipString(ipStr);
                        if (IsExternalAccessibleIP(ipString)) {
                            if (IsPublicIP(ipString)) {
                                publicIPs.push_back(ipString);
                            } else {
                                privateIPs.push_back(ipString);
                            }
                        }
                    }
                }
            }
            freeaddrinfo(result);
        }
    }

    WSACleanup();

    // 先返回公网IP，再返回私有IP
    std::vector<std::string> result;
    result.insert(result.end(), publicIPs.begin(), publicIPs.end());
    result.insert(result.end(), privateIPs.begin(), privateIPs.end());
    return result;
}

// 显示服务器连接信息
static void PrintServerConnectionInfo(uint16_t port) {
    std::cout << "\n";
    printLogo();
    std::cout << "\n";
    std::cout << "Server is ready for client connections:\n";
    std::cout << "\n";

    std::vector<std::string> externalIPs = GetExternalAccessibleIPs();

    if (externalIPs.empty()) {
        std::cout << "  [WARNING] No external accessible IP addresses found.\n";
        std::cout << "  Only localhost (127.0.0.1) is available for local connections.\n";
        std::cout << "\n";
        std::cout << "  Local only:  127.0.0.1:" << port << " (Local access only)\n";
    } else {
        // 分类显示
        bool hasPublic = false;
        bool hasPrivate = false;

        for (const auto& ip : externalIPs) {
            if (IsPublicIP(ip)) {
                hasPublic = true;
                break;
            } else {
                hasPrivate = true;
            }
        }

        if (hasPublic) {
            std::cout << "  Public IP (Internet access):\n";
            for (const auto& ip : externalIPs) {
                if (IsPublicIP(ip)) {
                    std::cout << "    " << ip << ":" << port << " (Internet access)\n";
                }
            }
            std::cout << "\n";
        }

        if (hasPrivate) {
            std::cout << "  Private IP (LAN access):\n";
            for (const auto& ip : externalIPs) {
                if (!IsPublicIP(ip)) {
                    std::cout << "    " << ip << ":" << port << " (LAN access)\n";
                }
            }
            std::cout << "\n";
        }

        std::cout << "  Local only:  127.0.0.1:" << port << " (Local access only)\n";
    }

    std::cout << "\n";
    std::cout << "Note: External clients should use Public IP or Private IP addresses.\n";
    std::cout << "========================================\n";
    std::cout << "\n";
}

int main() {
    // 设置控制台代码页为 UTF-8，以支持中文显示
    SetConsoleOutputCP(65001); // UTF-8 code page
    SetConsoleCP(65001);       // UTF-8 code page

    // 确保输出立即显示
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    SharedMemoryPool smp;
    g_smp = &smp;

    // 注册信号处理器
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    std::signal(SIGBREAK, SignalHandler);

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

    // 显示服务器连接信息（默认端口8888）
    const uint16_t kDefaultPort = 8888;
    PrintServerConnectionInfo(kDefaultPort);

    // 创建并启动 TCP 服务器（在后台线程中运行）
    TCPServer tcpServer(smp, kDefaultPort);
    g_tcp_server = &tcpServer;

    std::thread tcpServerThread([&tcpServer]() {
        if (!tcpServer.Start()) {
            std::cerr << "Failed to start TCP server on port " << kDefaultPort << "\n";
        }
    });

    // 分离线程，让它在后台运行
    tcpServerThread.detach();

    std::cout << "Shared Memory Management Server - type 'help' for commands\n";
    std::cout << "TCP server is listening on port " << kDefaultPort << " for client connections.\n";
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
            std::cout << "Stopping TCP server...\n";
            if (g_tcp_server != nullptr) {
                g_tcp_server->Stop();
            }
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
        std::cout << std::endl;
    }

    // 清理
    g_tcp_server = nullptr;
    g_smp = nullptr;
    return 0;
}
