#include "tcp_server.h"
#include "protocol.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
typedef int socklen_t;
#define close closesocket
#define SHUT_RDWR SD_BOTH
#endif

TCPServer::TCPServer(SharedMemoryPool& smp, uint16_t port)
    : smp_(smp), port_(port), listenSocket_(INVALID_SOCKET), running_(false) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

TCPServer::~TCPServer() {
    Stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool TCPServer::Start() {
    if (running_) {
        return false;
    }

    // 创建套接字
    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        return false;
    }

    // 设置套接字选项（允许地址重用）
    int opt = 1;
#ifdef _WIN32
    setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, 
               reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    // 绑定地址
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(listenSocket_, reinterpret_cast<sockaddr*>(&serverAddr), 
             sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind to port " << port_ << "\n";
        close(listenSocket_);
        return false;
    }

    // 开始监听
    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on port " << port_ << "\n";
        close(listenSocket_);
        return false;
    }

    running_ = true;
    std::cout << "TCP Server started on port " << port_ << "\n";

    // 接受连接循环
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        SOCKET clientSocket = accept(listenSocket_, 
                                     reinterpret_cast<sockaddr*>(&clientAddr), 
                                     &clientAddrLen);
        
        if (clientSocket == INVALID_SOCKET) {
            if (running_) {
                std::cerr << "Failed to accept connection\n";
            }
            continue;
        }

        // 获取客户端IP和端口
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);
        uint16_t clientPort = ntohs(clientAddr.sin_port);
        std::string clientIP(ipStr);
        std::string userID = Protocol::MakeUserID(clientIP, clientPort);

        std::cout << "Client connected: " << userID << "\n";

        // 为每个客户端创建处理线程
        std::lock_guard<std::mutex> lock(threadsMutex_);
        clientThreads_.emplace_back([this, clientSocket, clientIP, clientPort, userID]() {
            HandleClient(clientSocket, clientIP, clientPort);
        });
    }

    return true;
}

void TCPServer::Stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    // 关闭监听套接字
    if (listenSocket_ != INVALID_SOCKET) {
        shutdown(listenSocket_, SHUT_RDWR);
        close(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
    }

    // 等待所有客户端线程结束
    {
        std::lock_guard<std::mutex> lock(threadsMutex_);
        for (auto& thread : clientThreads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        clientThreads_.clear();
    }

    std::cout << "TCP Server stopped\n";
}

void TCPServer::HandleClient(SOCKET clientSocket, const std::string& clientIP, uint16_t clientPort) {
    std::string userID = Protocol::MakeUserID(clientIP, clientPort);

    while (running_) {
        Protocol::Request req;
        if (!ReceiveRequest(clientSocket, req)) {
            break; // 连接断开或错误
        }

        Protocol::Response resp;
        ProcessRequest(clientSocket, userID, req, resp);

        if (!SendResponse(clientSocket, resp)) {
            break; // 发送失败，断开连接
        }
    }

    // 清理：释放该用户的内存（可选）
    // smp_.FreeByUser(userID);

    close(clientSocket);
    std::cout << "Client disconnected: " << userID << "\n";
}

bool TCPServer::ReceiveRequest(SOCKET clientSocket, Protocol::Request& req) {
    // 先读取5字节（命令类型 + 数据长度）
    uint8_t header[5];
    size_t received = 0;
    
    while (received < 5) {
        int n = recv(clientSocket, reinterpret_cast<char*>(header + received), 
                     5 - received, 0);
        if (n <= 0) {
            return false; // 连接断开或错误
        }
        received += n;
    }

    // 解析数据长度
    uint32_t netLen;
    std::memcpy(&netLen, header + 1, 4);
    uint32_t dataLen = ntohl(netLen);

    if (dataLen > 1024 * 1024) { // 最大1MB
        return false;
    }

    // 读取数据内容
    std::vector<uint8_t> data(dataLen);
    received = 0;
    while (received < dataLen) {
        int n = recv(clientSocket, reinterpret_cast<char*>(data.data() + received), 
                     dataLen - received, 0);
        if (n <= 0) {
            return false;
        }
        received += n;
    }

    // 反序列化请求
    std::vector<uint8_t> fullPacket(5 + dataLen);
    std::memcpy(fullPacket.data(), header, 5);
    std::memcpy(fullPacket.data() + 5, data.data(), dataLen);

    return Protocol::DeserializeRequest(fullPacket.data(), fullPacket.size(), req);
}

void TCPServer::ProcessRequest(SOCKET clientSocket, const std::string& userID,
                               const Protocol::Request& req, Protocol::Response& resp) {
    resp.code = Protocol::ResponseCode::SUCCESS;
    resp.data.clear();

    try {
        switch (req.cmd) {
            case Protocol::CommandType::ALLOC: {
                // 数据格式：内容字符串（以\0结尾）
                if (req.data.empty()) {
                    resp.code = Protocol::ResponseCode::ERROR_INVALID_PARAM;
                    resp.data = "Empty content";
                    break;
                }
                
                int blockID = smp_.AllocateBlock(userID, req.data.data(), req.data.size());
                if (blockID < 0) {
                    resp.code = Protocol::ResponseCode::ERROR_NO_MEMORY;
                    resp.data = "Memory allocation failed";
                } else {
                    resp.data = "Allocated at block_" + std::to_string(blockID);
                }
                break;
            }

            case Protocol::CommandType::UPDATE: {
                if (req.data.empty()) {
                    resp.code = Protocol::ResponseCode::ERROR_INVALID_PARAM;
                    resp.data = "Empty content";
                    break;
                }
                
                // 检查用户是否存在
                const auto& userInfo = smp_.GetUserBlockInfo();
                if (userInfo.find(userID) == userInfo.end()) {
                    resp.code = Protocol::ResponseCode::ERROR_NOT_FOUND;
                    resp.data = "User not found";
                    break;
                }
                
                // 释放旧内存
                smp_.FreeByUser(userID);
                
                // 分配新内存
                int blockID = smp_.AllocateBlock(userID, req.data.data(), req.data.size());
                if (blockID < 0) {
                    resp.code = Protocol::ResponseCode::ERROR_NO_MEMORY;
                    resp.data = "Memory allocation failed";
                } else {
                    resp.data = "Updated at block_" + std::to_string(blockID);
                }
                break;
            }

            case Protocol::CommandType::DELETE: {
                if (!smp_.FreeByUser(userID)) {
                    resp.code = Protocol::ResponseCode::ERROR_NOT_FOUND;
                    resp.data = "User not found";
                } else {
                    resp.data = "Memory freed";
                }
                break;
            }

            case Protocol::CommandType::READ: {
                std::string content = smp_.GetUserContentAsString(userID);
                if (content.empty()) {
                    resp.code = Protocol::ResponseCode::ERROR_NOT_FOUND;
                    resp.data = "User not found or content is empty";
                } else {
                    resp.data = content;
                }
                break;
            }

            case Protocol::CommandType::STATUS: {
                // 返回用户的状态信息（JSON格式或简单文本）
                const auto& userInfo = smp_.GetUserBlockInfo();
                auto it = userInfo.find(userID);
                if (it == userInfo.end()) {
                    resp.code = Protocol::ResponseCode::ERROR_NOT_FOUND;
                    resp.data = "User not found";
                } else {
                    std::ostringstream oss;
                    oss << "User: " << userID << "\n";
                    oss << "Blocks: " << it->second.first << " - " 
                        << (it->second.first + it->second.second - 1) << "\n";
                    oss << "Last Modified: " << smp_.GetUserLastModifiedTimeString(userID);
                    resp.data = oss.str();
                }
                break;
            }

            case Protocol::CommandType::PING: {
                resp.data = "PONG";
                break;
            }

            default:
                resp.code = Protocol::ResponseCode::ERROR_INVALID_CMD;
                resp.data = "Unknown command";
                break;
        }
    } catch (const std::exception& e) {
        resp.code = Protocol::ResponseCode::ERROR_INTERNAL;
        resp.data = std::string("Internal error: ") + e.what();
    }
}

bool TCPServer::SendResponse(SOCKET clientSocket, const Protocol::Response& resp) {
    std::vector<uint8_t> buffer = Protocol::SerializeResponse(resp);
    
    size_t sent = 0;
    while (sent < buffer.size()) {
        int n = send(clientSocket, reinterpret_cast<const char*>(buffer.data() + sent),
                     buffer.size() - sent, 0);
        if (n <= 0) {
            return false; // 发送失败
        }
        sent += n;
    }
    
    return true;
}
