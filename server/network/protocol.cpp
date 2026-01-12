#include "protocol.h"
#include <vector>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace Protocol {

// 网络字节序转换（大端序）
static uint32_t HostToNetwork32(uint32_t value) {
    return htonl(value);
}

static uint32_t NetworkToHost32(uint32_t value) {
    return ntohl(value);
}

std::vector<uint8_t> SerializeRequest(const Request& req) {
    std::vector<uint8_t> buffer;
    buffer.reserve(5 + req.data.size());
    
    // 命令类型（1字节）
    buffer.push_back(static_cast<uint8_t>(req.cmd));
    
    // 数据长度（4字节，大端序）
    uint32_t len = static_cast<uint32_t>(req.data.size());
    uint32_t netLen = HostToNetwork32(len);
    const uint8_t* lenBytes = reinterpret_cast<const uint8_t*>(&netLen);
    buffer.insert(buffer.end(), lenBytes, lenBytes + 4);
    
    // 数据内容
    buffer.insert(buffer.end(), req.data.begin(), req.data.end());
    
    return buffer;
}

bool DeserializeRequest(const uint8_t* buffer, size_t len, Request& req) {
    if (len < 5) {
        return false; // 至少需要5字节（1字节命令 + 4字节长度）
    }
    
    // 读取命令类型
    req.cmd = static_cast<CommandType>(buffer[0]);
    
    // 读取数据长度（大端序）
    uint32_t netLen;
    std::memcpy(&netLen, buffer + 1, 4);
    uint32_t dataLen = NetworkToHost32(netLen);
    
    // 检查长度是否合理
    if (dataLen > 1024 * 1024) { // 最大1MB
        return false;
    }
    
    // 检查缓冲区是否足够
    if (len < 5 + dataLen) {
        return false;
    }
    
    // 读取数据内容
    req.data.assign(reinterpret_cast<const char*>(buffer + 5), dataLen);
    
    return true;
}

std::vector<uint8_t> SerializeResponse(const Response& resp) {
    std::vector<uint8_t> buffer;
    buffer.reserve(5 + resp.data.size());
    
    // 状态码（1字节）
    buffer.push_back(static_cast<uint8_t>(resp.code));
    
    // 数据长度（4字节，大端序）
    uint32_t len = static_cast<uint32_t>(resp.data.size());
    uint32_t netLen = HostToNetwork32(len);
    const uint8_t* lenBytes = reinterpret_cast<const uint8_t*>(&netLen);
    buffer.insert(buffer.end(), lenBytes, lenBytes + 4);
    
    // 数据内容
    buffer.insert(buffer.end(), resp.data.begin(), resp.data.end());
    
    return buffer;
}

bool DeserializeResponse(const uint8_t* buffer, size_t len, Response& resp) {
    if (len < 5) {
        return false;
    }
    
    // 读取状态码
    resp.code = static_cast<ResponseCode>(buffer[0]);
    
    // 读取数据长度（大端序）
    uint32_t netLen;
    std::memcpy(&netLen, buffer + 1, 4);
    uint32_t dataLen = NetworkToHost32(netLen);
    
    if (dataLen > 1024 * 1024) {
        return false;
    }
    
    if (len < 5 + dataLen) {
        return false;
    }
    
    // 读取数据内容
    resp.data.assign(reinterpret_cast<const char*>(buffer + 5), dataLen);
    
    return true;
}

std::string MakeUserID(const std::string& ip, uint16_t port) {
    std::ostringstream oss;
    oss << ip << ":" << port;
    return oss.str();
}

bool ParseUserID(const std::string& userID, std::string& ip, uint16_t& port) {
    size_t colonPos = userID.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }
    
    ip = userID.substr(0, colonPos);
    std::string portStr = userID.substr(colonPos + 1);
    
    try {
        port = static_cast<uint16_t>(std::stoul(portStr));
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace Protocol
