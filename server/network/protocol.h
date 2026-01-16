#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Protocol {
// 命令类型定义
enum class CommandType : uint8_t {
    ALLOC = 0x01,  // 分配内存
    UPDATE = 0x02, // 更新内容
    DELETE = 0x03, // 删除/释放内存
    READ = 0x04,   // 读取内容
    STATUS = 0x05  // 查询状态
};

// 响应状态码
enum class ResponseCode : uint8_t {
    SUCCESS = 0x00,              // 成功
    ERROR_INVALID_CMD = 0x01,    // 无效命令
    ERROR_INVALID_PARAM = 0x02,  // 无效参数
    ERROR_NO_MEMORY = 0x03,      // 内存不足
    ERROR_NOT_FOUND = 0x04,      // 用户不存在
    ERROR_ALREADY_EXISTS = 0x05, // 用户已存在
    ERROR_INTERNAL = 0xFF        // 内部错误
};

// 请求包结构
struct Request {
    CommandType cmd;  // 命令类型（1字节）
    uint32_t dataLen; // 数据长度（4字节，大端序）
    std::string data; // 数据内容（N字节）
};

// 响应包结构
struct Response {
    ResponseCode code; // 状态码（1字节）
    uint32_t dataLen;  // 数据长度（4字节，大端序）
    std::string data;  // 响应数据（N字节）
};

// 序列化请求到字节流
std::vector<uint8_t> SerializeRequest(const Request& req);

// 从字节流反序列化请求
bool DeserializeRequest(const uint8_t* buffer, size_t len, Request& req);

// 序列化响应到字节流
std::vector<uint8_t> SerializeResponse(const Response& resp);

// 从字节流反序列化响应
bool DeserializeResponse(const uint8_t* buffer, size_t len, Response& resp);

// 将IP地址和端口号组合成用户标识
std::string MakeUserID(const std::string& ip, uint16_t port);

// 解析用户标识，提取IP和端口
bool ParseUserID(const std::string& userID, std::string& ip, uint16_t& port);
} // namespace Protocol
