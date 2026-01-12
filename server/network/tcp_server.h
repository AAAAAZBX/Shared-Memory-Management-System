#pragma once
#include "../shared_memory_pool/shared_memory_pool.h"
#include "protocol.h"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

class TCPServer {
  public:
    TCPServer(SharedMemoryPool& smp, uint16_t port = 8888);
    ~TCPServer();

    // 启动服务器
    bool Start();

    // 停止服务器
    void Stop();

    // 检查服务器是否运行
    bool IsRunning() const {
        return running_;
    }

  private:
    // 客户端连接处理函数（每个连接一个线程）
    void HandleClient(SOCKET clientSocket, const std::string& clientIP, uint16_t clientPort);

    // 处理客户端请求（使用 Memory ID 系统）
    void ProcessRequest(SOCKET clientSocket, const Protocol::Request& req,
                        Protocol::Response& resp);

    // 发送响应
    bool SendResponse(SOCKET clientSocket, const Protocol::Response& resp);

    // 接收完整请求（处理粘包问题）
    bool ReceiveRequest(SOCKET clientSocket, Protocol::Request& req);

  private:
    SharedMemoryPool& smp_;                  // 内存池引用
    uint16_t port_;                          // 监听端口
    SOCKET listenSocket_;                    // 监听套接字
    std::atomic<bool> running_;              // 服务器运行状态
    std::vector<std::thread> clientThreads_; // 客户端处理线程
    std::mutex threadsMutex_;                // 线程列表互斥锁
};
