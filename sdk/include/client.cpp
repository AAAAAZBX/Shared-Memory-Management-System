#include "client_sdk.h"

int main() {
    SMMClient::ClientSDK client("127.0.0.1", 8888);
    client.StartInteractiveCLI(); // 启动交互式命令行
    return 0;
}