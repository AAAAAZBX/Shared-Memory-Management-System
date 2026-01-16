// 客户端命令行工具示例
// 演示如何使用 ClientSDK 进行交互式操作

#include "../include/client_sdk.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
    // 设置控制台代码页为 UTF-8（Windows）
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    std::string host = "127.0.0.1";
    uint16_t port = 8888;

    // 解析命令行参数
    if (argc >= 2) {
        host = argv[1];
    }
    if (argc >= 3) {
        port = static_cast<uint16_t>(std::stoi(argv[2]));
    }

    // 创建客户端 SDK
    SMMClient::ClientSDK client(host, port);

    // 启动交互式命令行
    client.StartInteractiveCLI();

    return 0;
}
