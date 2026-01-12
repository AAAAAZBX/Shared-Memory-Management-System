#include "tcp_server.h"
#include "protocol.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>

typedef int socklen_t;
#define close closesocket
#define SHUT_RDWR SD_BOTH

TCPServer::TCPServer(SharedMemoryPool& smp, uint16_t port)
    : smp_(smp), port_(port), listenSocket_(INVALID_SOCKET), running_(false) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

TCPServer::~TCPServer() {
    Stop();
    WSACleanup();
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
    setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt),
               sizeof(opt));

    // 绑定地址
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(listenSocket_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) ==
        SOCKET_ERROR) {
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

        SOCKET clientSocket =
            accept(listenSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);

        if (clientSocket == INVALID_SOCKET) {
            if (running_) {
                std::cerr << "Failed to accept connection\n";
            }
            continue;
        }

        // 获取客户端IP和端口
        char ipStr[INET_ADDRSTRLEN];
        // Windows 上使用 inet_ntoa（更兼容）
        const char* ipCStr = inet_ntoa(clientAddr.sin_addr);
        if (ipCStr != nullptr) {
            std::strncpy(ipStr, ipCStr, INET_ADDRSTRLEN - 1);
            ipStr[INET_ADDRSTRLEN - 1] = '\0';
        } else {
            std::strcpy(ipStr, "unknown");
        }
        uint16_t clientPort = ntohs(clientAddr.sin_port);
        std::string clientIP(ipStr);
        std::string clientInfo = clientIP + ":" + std::to_string(clientPort);

        std::cout << "Client connected: " << clientInfo << "\n";

        // 为每个客户端创建处理线程
        std::lock_guard<std::mutex> lock(threadsMutex_);
        clientThreads_.emplace_back([this, clientSocket, clientIP, clientPort]() {
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

void TCPServer::HandleClient(SOCKET clientSocket, const std::string& clientIP,
                             uint16_t clientPort) {
    std::string clientInfo = clientIP + ":" + std::to_string(clientPort);

    while (running_) {
        Protocol::Request req;
        if (!ReceiveRequest(clientSocket, req)) {
            break; // 连接断开或错误
        }

        Protocol::Response resp;
        ProcessRequest(clientSocket, req, resp);

        if (!SendResponse(clientSocket, resp)) {
            break; // 发送失败，断开连接
        }
    }

    // 注意：客户端断开时不会自动清理内存，内存由 Memory ID 管理

    close(clientSocket);
    std::cout << "Client disconnected: " << clientInfo << "\n";
}

bool TCPServer::ReceiveRequest(SOCKET clientSocket, Protocol::Request& req) {
    // 先读取5字节（命令类型 + 数据长度）
    uint8_t header[5];
    size_t received = 0;

    while (received < 5) {
        int n = recv(clientSocket, reinterpret_cast<char*>(header + received), 5 - received, 0);
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

void TCPServer::ProcessRequest(SOCKET clientSocket, const Protocol::Request& req,
                               Protocol::Response& resp) {
    resp.code = Protocol::ResponseCode::SUCCESS;
    resp.data.clear();

    try {
        switch (req.cmd) {
        case Protocol::CommandType::ALLOC: {
            // 数据格式：description\0content
            if (req.data.empty()) {
                resp.code = Protocol::ResponseCode::ERROR_INVALID_PARAM;
                resp.data = "Empty data, expected format: description\\0content";
                break;
            }

            // 查找分隔符
            size_t nullPos = req.data.find('\0');
            if (nullPos == std::string::npos || nullPos == 0) {
                resp.code = Protocol::ResponseCode::ERROR_INVALID_PARAM;
                resp.data = "Invalid format, expected: description\\0content";
                break;
            }

            std::string description = req.data.substr(0, nullPos);
            std::string content = req.data.substr(nullPos + 1);

            if (content.empty()) {
                resp.code = Protocol::ResponseCode::ERROR_INVALID_PARAM;
                resp.data = "Empty content";
                break;
            }

            // 自动生成 Memory ID
            std::string memory_id = smp_.GenerateNextMemoryId();

            // 分配内存
            int blockID =
                smp_.AllocateBlock(memory_id, description, content.data(), content.size());
            if (blockID < 0) {
                resp.code = Protocol::ResponseCode::ERROR_NO_MEMORY;
                resp.data = "Memory allocation failed";
            } else {
                resp.data = memory_id; // 返回 Memory ID
            }
            break;
        }

        case Protocol::CommandType::UPDATE: {
            // 数据格式：memory_id\0new_content
            if (req.data.empty()) {
                resp.code = Protocol::ResponseCode::ERROR_INVALID_PARAM;
                resp.data = "Empty data, expected format: memory_id\\0new_content";
                break;
            }

            size_t nullPos = req.data.find('\0');
            if (nullPos == std::string::npos || nullPos == 0) {
                resp.code = Protocol::ResponseCode::ERROR_INVALID_PARAM;
                resp.data = "Invalid format, expected: memory_id\\0new_content";
                break;
            }

            std::string memory_id = req.data.substr(0, nullPos);
            std::string newContent = req.data.substr(nullPos + 1);

            // 检查 Memory ID 是否存在
            const auto& memoryInfo = smp_.GetMemoryInfo();
            if (memoryInfo.find(memory_id) == memoryInfo.end()) {
                resp.code = Protocol::ResponseCode::ERROR_NOT_FOUND;
                resp.data = "Memory ID not found: " + memory_id;
                break;
            }

            // 获取当前描述
            const auto& meta = smp_.GetMeta(memoryInfo.at(memory_id).first);
            std::string description = meta.description;

            // 释放旧内存
            smp_.FreeByMemoryId(memory_id);

            // 分配新内存
            int blockID =
                smp_.AllocateBlock(memory_id, description, newContent.data(), newContent.size());
            if (blockID < 0) {
                resp.code = Protocol::ResponseCode::ERROR_NO_MEMORY;
                resp.data = "Memory allocation failed";
            } else {
                resp.data = "Updated: " + memory_id;
            }
            break;
        }

        case Protocol::CommandType::DELETE: {
            // 数据格式：memory_id
            if (req.data.empty()) {
                resp.code = Protocol::ResponseCode::ERROR_INVALID_PARAM;
                resp.data = "Empty memory_id";
                break;
            }

            std::string memory_id = req.data;
            if (!smp_.FreeByMemoryId(memory_id)) {
                resp.code = Protocol::ResponseCode::ERROR_NOT_FOUND;
                resp.data = "Memory ID not found: " + memory_id;
            } else {
                resp.data = "Memory freed: " + memory_id;
            }
            break;
        }

        case Protocol::CommandType::READ: {
            // 数据格式：memory_id
            if (req.data.empty()) {
                resp.code = Protocol::ResponseCode::ERROR_INVALID_PARAM;
                resp.data = "Empty memory_id";
                break;
            }

            std::string memory_id = req.data;
            std::string content = smp_.GetMemoryContentAsString(memory_id);
            if (content.empty()) {
                resp.code = Protocol::ResponseCode::ERROR_NOT_FOUND;
                resp.data = "Memory ID not found or content is empty: " + memory_id;
            } else {
                // 返回格式化的内容信息
                const auto& memoryInfo = smp_.GetMemoryInfo();
                auto it = memoryInfo.find(memory_id);
                if (it != memoryInfo.end()) {
                    const auto& meta = smp_.GetMeta(it->second.first);
                    std::ostringstream oss;
                    oss << "Memory ID: " << memory_id << "\n";
                    oss << "Description: " << meta.description << "\n";
                    oss << "Content: " << content << "\n";
                    oss << "Size: " << content.size() << " bytes\n";
                    oss << "Last Modified: " << smp_.GetMemoryLastModifiedTimeString(memory_id);
                    resp.data = oss.str();
                } else {
                    resp.data = content;
                }
            }
            break;
        }

        case Protocol::CommandType::STATUS: {
            // 返回所有内存块的状态信息
            const auto& memoryInfo = smp_.GetMemoryInfo();
            if (memoryInfo.empty()) {
                resp.data = "No allocated memory blocks";
            } else {
                std::ostringstream oss;
                oss << "Memory Pool Status:\n";
                oss << "Total blocks: " << memoryInfo.size() << "\n\n";

                for (const auto& entry : memoryInfo) {
                    const auto& meta = smp_.GetMeta(entry.second.first);
                    oss << "Memory ID: " << entry.first << "\n";
                    oss << "  Description: " << meta.description << "\n";
                    oss << "  Blocks: " << entry.second.first << " - "
                        << (entry.second.first + entry.second.second - 1) << "\n";
                    oss << "  Last Modified: " << smp_.GetMemoryLastModifiedTimeString(entry.first)
                        << "\n\n";
                }
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
