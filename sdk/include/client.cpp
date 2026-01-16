#include "client_sdk.h"

int main() {
    SMMClient::ClientSDK client("192.168.20.31", 8888);
    client.StartInteractiveCLI(); // 启动交互式命令行
    return 0;
}